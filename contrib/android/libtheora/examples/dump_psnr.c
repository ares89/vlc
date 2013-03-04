/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggTheora SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE Theora SOURCE CODE IS COPYRIGHT (C) 2002-2009                *
 * by the Xiph.Org Foundation and contributors http://www.xiph.org/ *
 *                                                                  *
 ********************************************************************

  function: example dumpvid application; dumps  Theora streams
  last mod: $Id$

 ********************************************************************/

#if !defined(_GNU_SOURCE)
#define _GNU_SOURCE
#endif
#if !defined(_LARGEFILE_SOURCE)
#define _LARGEFILE_SOURCE
#endif
#if !defined(_LARGEFILE64_SOURCE)
#define _LARGEFILE64_SOURCE
#endif
#if !defined(_FILE_OFFSET_BITS)
#define _FILE_OFFSET_BITS 64
#endif

#include <stdio.h>
#if !defined(_WIN32)
#include <getopt.h>
#include <unistd.h>
#else
#include "getopt.h"
#endif
#include <stdlib.h>
#include <string.h>
#include <sys/timeb.h>
#include <sys/types.h>
#include <sys/stat.h>
/*Yes, yes, we're going to hell.*/
#if defined(_WIN32)
#include <io.h>
#endif
#include <fcntl.h>
#include <math.h>
#include <signal.h>
#include "theora/theoradec.h"

const char *optstring = "fsy";
struct option options [] = {
  {"frame-type",no_argument,NULL,'f'},
  {"summary",no_argument,NULL,'s'},
  {"luma-only",no_argument,NULL,'y'},
  {NULL,0,NULL,0}
};

static int show_frame_type;
static int summary_only;
static int luma_only;

typedef struct y4m_input y4m_input;

/*The function used to perform chroma conversion.*/
typedef void (*y4m_convert_func)(y4m_input *_y4m,
 unsigned char *_dst,unsigned char *_aux);

struct y4m_input{
  int               frame_w;
  int               frame_h;
  int               pic_w;
  int               pic_h;
  int               pic_x;
  int               pic_y;
  int               fps_n;
  int               fps_d;
  int               par_n;
  int               par_d;
  char              interlace;
  int               src_c_dec_h;
  int               src_c_dec_v;
  int               dst_c_dec_h;
  int               dst_c_dec_v;
  char              chroma_type[16];
  /*The size of each converted frame buffer.*/
  size_t            dst_buf_sz;
  /*The amount to read directly into the converted frame buffer.*/
  size_t            dst_buf_read_sz;
  /*The size of the auxilliary buffer.*/
  size_t            aux_buf_sz;
  /*The amount to read into the auxilliary buffer.*/
  size_t            aux_buf_read_sz;
  y4m_convert_func  convert;
  unsigned char    *dst_buf;
  unsigned char    *aux_buf;
};


static int y4m_parse_tags(y4m_input *_y4m,char *_tags){
  int   got_w;
  int   got_h;
  int   got_fps;
  int   got_interlace;
  int   got_par;
  int   got_chroma;
  char *p;
  char *q;
  got_w=got_h=got_fps=got_interlace=got_par=got_chroma=0;
  for(p=_tags;;p=q){
    /*Skip any leading spaces.*/
    while(*p==' ')p++;
    /*If that's all we have, stop.*/
    if(p[0]=='\0')break;
    /*Find the end of this tag.*/
    for(q=p+1;*q!='\0'&&*q!=' ';q++);
    /*Process the tag.*/
    switch(p[0]){
      case 'W':{
        if(sscanf(p+1,"%d",&_y4m->pic_w)!=1)return -1;
        got_w=1;
      }break;
      case 'H':{
        if(sscanf(p+1,"%d",&_y4m->pic_h)!=1)return -1;
        got_h=1;
      }break;
      case 'F':{
        if(sscanf(p+1,"%d:%d",&_y4m->fps_n,&_y4m->fps_d)!=2){
          return -1;
        }
        got_fps=1;
      }break;
      case 'I':{
        _y4m->interlace=p[1];
        got_interlace=1;
      }break;
      case 'A':{
        if(sscanf(p+1,"%d:%d",&_y4m->par_n,&_y4m->par_d)!=2){
          return -1;
        }
        got_par=1;
      }break;
      case 'C':{
        if(q-p>16)return -1;
        memcpy(_y4m->chroma_type,p+1,q-p-1);
        _y4m->chroma_type[q-p-1]='\0';
        got_chroma=1;
      }break;
      /*Ignore unknown tags.*/
    }
  }
  if(!got_w||!got_h||!got_fps||!got_interlace||!got_par)return -1;
  /*Chroma-type is not specified in older files, e.g., those generated by
     mplayer.*/
  if(!got_chroma)strcpy(_y4m->chroma_type,"420");
  return 0;
}

/*All anti-aliasing filters in the following conversion functions are based on
   one of two window functions:
  The 6-tap Lanczos window (for down-sampling and shifts):
   sinc(\pi*t)*sinc(\pi*t/3), |t|<3  (sinc(t)==sin(t)/t)
   0,                         |t|>=3
  The 4-tap Mitchell window (for up-sampling):
   7|t|^3-12|t|^2+16/3,             |t|<1
   -(7/3)|x|^3+12|x|^2-20|x|+32/3,  |t|<2
   0,                               |t|>=2
  The number of taps is intentionally kept small to reduce computational
   overhead and limit ringing.

  The taps from these filters are scaled so that their sum is 1, and the result
   is scaled by 128 and rounded to integers to create a filter whose
   intermediate values fit inside 16 bits.
  Coefficients are rounded in such a way as to ensure their sum is still 128,
   which is usually equivalent to normal rounding.*/

#define OC_MINI(_a,_b)      ((_a)>(_b)?(_b):(_a))
#define OC_MAXI(_a,_b)      ((_a)<(_b)?(_b):(_a))
#define OC_CLAMPI(_a,_b,_c) (OC_MAXI(_a,OC_MINI(_b,_c)))

/*420jpeg chroma samples are sited like:
  Y-------Y-------Y-------Y-------
  |       |       |       |
  |   BR  |       |   BR  |
  |       |       |       |
  Y-------Y-------Y-------Y-------
  |       |       |       |
  |       |       |       |
  |       |       |       |
  Y-------Y-------Y-------Y-------
  |       |       |       |
  |   BR  |       |   BR  |
  |       |       |       |
  Y-------Y-------Y-------Y-------
  |       |       |       |
  |       |       |       |
  |       |       |       |

  420mpeg2 chroma samples are sited like:
  Y-------Y-------Y-------Y-------
  |       |       |       |
  BR      |       BR      |
  |       |       |       |
  Y-------Y-------Y-------Y-------
  |       |       |       |
  |       |       |       |
  |       |       |       |
  Y-------Y-------Y-------Y-------
  |       |       |       |
  BR      |       BR      |
  |       |       |       |
  Y-------Y-------Y-------Y-------
  |       |       |       |
  |       |       |       |
  |       |       |       |

  We use a resampling filter to shift the site locations one quarter pixel (at
   the chroma plane's resolution) to the right.
  The 4:2:2 modes look exactly the same, except there are twice as many chroma
   lines, and they are vertically co-sited with the luma samples in both the
   mpeg2 and jpeg cases (thus requiring no vertical resampling).*/
static void y4m_convert_42xmpeg2_42xjpeg(y4m_input *_y4m,unsigned char *_dst,
 unsigned char *_aux){
  int c_w;
  int c_h;
  int pli;
  int y;
  int x;
  /*Skip past the luma data.*/
  _dst+=_y4m->pic_w*_y4m->pic_h;
  /*Compute the size of each chroma plane.*/
  c_w=(_y4m->pic_w+_y4m->dst_c_dec_h-1)/_y4m->dst_c_dec_h;
  c_h=(_y4m->pic_h+_y4m->dst_c_dec_v-1)/_y4m->dst_c_dec_v;
  for(pli=1;pli<3;pli++){
    for(y=0;y<c_h;y++){
      /*Filter: [4 -17 114 35 -9 1]/128, derived from a 6-tap Lanczos
         window.*/
      for(x=0;x<OC_MINI(c_w,2);x++){
        _dst[x]=(unsigned char)OC_CLAMPI(0,4*_aux[0]-17*_aux[OC_MAXI(x-1,0)]+
         114*_aux[x]+35*_aux[OC_MINI(x+1,c_w-1)]-9*_aux[OC_MINI(x+2,c_w-1)]+
         _aux[OC_MINI(x+3,c_w-1)]+64>>7,255);
      }
      for(;x<c_w-3;x++){
        _dst[x]=(unsigned char)OC_CLAMPI(0,4*_aux[x-2]-17*_aux[x-1]+
         114*_aux[x]+35*_aux[x+1]-9*_aux[x+2]+_aux[x+3]+64>>7,255);
      }
      for(;x<c_w;x++){
        _dst[x]=(unsigned char)OC_CLAMPI(0,4*_aux[x-2]-17*_aux[x-1]+
         114*_aux[x]+35*_aux[OC_MINI(x+1,c_w-1)]-9*_aux[OC_MINI(x+2,c_w-1)]+
         _aux[c_w-1]+64>>7,255);
      }
      _dst+=c_w;
      _aux+=c_w;
    }
  }
}

/*This format is only used for interlaced content, but is included for
   completeness.

  420jpeg chroma samples are sited like:
  Y-------Y-------Y-------Y-------
  |       |       |       |
  |   BR  |       |   BR  |
  |       |       |       |
  Y-------Y-------Y-------Y-------
  |       |       |       |
  |       |       |       |
  |       |       |       |
  Y-------Y-------Y-------Y-------
  |       |       |       |
  |   BR  |       |   BR  |
  |       |       |       |
  Y-------Y-------Y-------Y-------
  |       |       |       |
  |       |       |       |
  |       |       |       |

  420paldv chroma samples are sited like:
  YR------Y-------YR------Y-------
  |       |       |       |
  |       |       |       |
  |       |       |       |
  YB------Y-------YB------Y-------
  |       |       |       |
  |       |       |       |
  |       |       |       |
  YR------Y-------YR------Y-------
  |       |       |       |
  |       |       |       |
  |       |       |       |
  YB------Y-------YB------Y-------
  |       |       |       |
  |       |       |       |
  |       |       |       |

  We use a resampling filter to shift the site locations one quarter pixel (at
   the chroma plane's resolution) to the right.
  Then we use another filter to move the C_r location down one quarter pixel,
   and the C_b location up one quarter pixel.*/
static void y4m_convert_42xpaldv_42xjpeg(y4m_input *_y4m,unsigned char *_dst,
 unsigned char *_aux){
  unsigned char *tmp;
  int            c_w;
  int            c_h;
  int            c_sz;
  int            pli;
  int            y;
  int            x;
  /*Skip past the luma data.*/
  _dst+=_y4m->pic_w*_y4m->pic_h;
  /*Compute the size of each chroma plane.*/
  c_w=(_y4m->pic_w+1)/2;
  c_h=(_y4m->pic_h+_y4m->dst_c_dec_h-1)/_y4m->dst_c_dec_h;
  c_sz=c_w*c_h;
  /*First do the horizontal re-sampling.
    This is the same as the mpeg2 case, except that after the horizontal case,
     we need to apply a second vertical filter.*/
  tmp=_aux+2*c_sz;
  for(pli=1;pli<3;pli++){
    for(y=0;y<c_h;y++){
      /*Filter: [4 -17 114 35 -9 1]/128, derived from a 6-tap Lanczos
         window.*/
      for(x=0;x<OC_MINI(c_w,2);x++){
        tmp[x]=(unsigned char)OC_CLAMPI(0,4*_aux[0]-17*_aux[OC_MAXI(x-1,0)]+
         114*_aux[x]+35*_aux[OC_MINI(x+1,c_w-1)]-9*_aux[OC_MINI(x+2,c_w-1)]+
         _aux[OC_MINI(x+3,c_w-1)]+64>>7,255);
      }
      for(;x<c_w-3;x++){
        tmp[x]=(unsigned char)OC_CLAMPI(0,4*_aux[x-2]-17*_aux[x-1]+
         114*_aux[x]+35*_aux[x+1]-9*_aux[x+2]+_aux[x+3]+64>>7,255);
      }
      for(;x<c_w;x++){
        tmp[x]=(unsigned char)OC_CLAMPI(0,4*_aux[x-2]-17*_aux[x-1]+
         114*_aux[x]+35*_aux[OC_MINI(x+1,c_w-1)]-9*_aux[OC_MINI(x+2,c_w-1)]+
         _aux[c_w-1]+64>>7,255);
      }
      tmp+=c_w;
      _aux+=c_w;
    }
    switch(pli){
      case 1:{
        tmp-=c_sz;
        /*Slide C_b up a quarter-pel.
          This is the same filter used above, but in the other order.*/
        for(x=0;x<c_w;x++){
          for(y=0;y<OC_MINI(c_h,3);y++){
            _dst[y*c_w]=(unsigned char)OC_CLAMPI(0,tmp[0]-
             9*tmp[OC_MAXI(y-2,0)*c_w]+35*tmp[OC_MAXI(y-1,0)*c_w]+
             114*tmp[y*c_w]-17*tmp[OC_MINI(y+1,c_h-1)*c_w]+
             4*tmp[OC_MINI(y+2,c_h-1)*c_w]+64>>7,255);
          }
          for(;y<c_h-2;y++){
            _dst[y*c_w]=(unsigned char)OC_CLAMPI(0,tmp[(y-3)*c_w]-
             9*tmp[(y-2)*c_w]+35*tmp[(y-1)*c_w]+114*tmp[y*c_w]-
             17*tmp[(y+1)*c_w]+4*tmp[(y+2)*c_w]+64>>7,255);
          }
          for(;y<c_h;y++){
            _dst[y*c_w]=(unsigned char)OC_CLAMPI(0,tmp[(y-3)*c_w]-
             9*tmp[(y-2)*c_w]+35*tmp[(y-1)*c_w]+114*tmp[y*c_w]-
             17*tmp[OC_MINI(y+1,c_h-1)*c_w]+4*tmp[(c_h-1)*c_w]+64>>7,255);
          }
          _dst++;
          tmp++;
        }
        _dst+=c_sz-c_w;
        tmp-=c_w;
      }break;
      case 2:{
        tmp-=c_sz;
        /*Slide C_r down a quarter-pel.
          This is the same as the horizontal filter.*/
        for(x=0;x<c_w;x++){
          for(y=0;y<OC_MINI(c_h,2);y++){
            _dst[y*c_w]=(unsigned char)OC_CLAMPI(0,4*tmp[0]-
             17*tmp[OC_MAXI(y-1,0)*c_w]+114*tmp[y*c_w]+
             35*tmp[OC_MINI(y+1,c_h-1)*c_w]-9*tmp[OC_MINI(y+2,c_h-1)*c_w]+
             tmp[OC_MINI(y+3,c_h-1)*c_w]+64>>7,255);
          }
          for(;y<c_h-3;y++){
            _dst[y*c_w]=(unsigned char)OC_CLAMPI(0,4*tmp[(y-2)*c_w]-
             17*tmp[(y-1)*c_w]+114*tmp[y*c_w]+35*tmp[(y+1)*c_w]-
             9*tmp[(y+2)*c_w]+tmp[(y+3)*c_w]+64>>7,255);
          }
          for(;y<c_h;y++){
            _dst[y*c_w]=(unsigned char)OC_CLAMPI(0,4*tmp[(y-2)*c_w]-
             17*tmp[(y-1)*c_w]+114*tmp[y*c_w]+35*tmp[OC_MINI(y+1,c_h-1)*c_w]-
             9*tmp[OC_MINI(y+2,c_h-1)*c_w]+tmp[(c_h-1)*c_w]+64>>7,255);
          }
          _dst++;
          tmp++;
        }
      }break;
    }
    /*For actual interlaced material, this would have to be done separately on
       each field, and the shift amounts would be different.
      C_r moves down 1/8, C_b up 3/8 in the top field, and C_r moves down 3/8,
       C_b up 1/8 in the bottom field.
      The corresponding filters would be:
       Down 1/8 (reverse order for up): [3 -11 125 15 -4 0]/128
       Down 3/8 (reverse order for up): [4 -19 98 56 -13 2]/128*/
  }
}

/*422jpeg chroma samples are sited like:
  Y---BR--Y-------Y---BR--Y-------
  |       |       |       |
  |       |       |       |
  |       |       |       |
  Y---BR--Y-------Y---BR--Y-------
  |       |       |       |
  |       |       |       |
  |       |       |       |
  Y---BR--Y-------Y---BR--Y-------
  |       |       |       |
  |       |       |       |
  |       |       |       |
  Y---BR--Y-------Y---BR--Y-------
  |       |       |       |
  |       |       |       |
  |       |       |       |

  411 chroma samples are sited like:
  YBR-----Y-------Y-------Y-------
  |       |       |       |
  |       |       |       |
  |       |       |       |
  YBR-----Y-------Y-------Y-------
  |       |       |       |
  |       |       |       |
  |       |       |       |
  YBR-----Y-------Y-------Y-------
  |       |       |       |
  |       |       |       |
  |       |       |       |
  YBR-----Y-------Y-------Y-------
  |       |       |       |
  |       |       |       |
  |       |       |       |

  We use a filter to resample at site locations one eighth pixel (at the source
   chroma plane's horizontal resolution) and five eighths of a pixel to the
   right.*/
static void y4m_convert_411_422jpeg(y4m_input *_y4m,unsigned char *_dst,
 unsigned char *_aux){
  int c_w;
  int dst_c_w;
  int c_h;
  int pli;
  int y;
  int x;
  /*Skip past the luma data.*/
  _dst+=_y4m->pic_w*_y4m->pic_h;
  /*Compute the size of each chroma plane.*/
  c_w=(_y4m->pic_w+_y4m->src_c_dec_h-1)/_y4m->src_c_dec_h;
  dst_c_w=(_y4m->pic_w+_y4m->dst_c_dec_h-1)/_y4m->dst_c_dec_h;
  c_h=(_y4m->pic_h+_y4m->dst_c_dec_v-1)/_y4m->dst_c_dec_v;
  for(pli=1;pli<3;pli++){
    for(y=0;y<c_h;y++){
      /*Filters: [1 110 18 -1]/128 and [-3 50 86 -5]/128, both derived from a
         4-tap Mitchell window.*/
      for(x=0;x<OC_MINI(c_w,1);x++){
        _dst[x<<1]=(unsigned char)OC_CLAMPI(0,111*_aux[0]+
         18*_aux[OC_MINI(1,c_w-1)]-_aux[OC_MINI(2,c_w-1)]+64>>7,255);
        _dst[x<<1|1]=(unsigned char)OC_CLAMPI(0,47*_aux[0]+
         86*_aux[OC_MINI(1,c_w-1)]-5*_aux[OC_MINI(2,c_w-1)]+64>>7,255);
      }
      for(;x<c_w-2;x++){
        _dst[x<<1]=(unsigned char)OC_CLAMPI(0,_aux[x-1]+110*_aux[x]+
         18*_aux[x+1]-_aux[x+2]+64>>7,255);
        _dst[x<<1|1]=(unsigned char)OC_CLAMPI(0,-3*_aux[x-1]+50*_aux[x]+
         86*_aux[x+1]-5*_aux[x+2]+64>>7,255);
      }
      for(;x<c_w;x++){
        _dst[x<<1]=(unsigned char)OC_CLAMPI(0,_aux[x-1]+110*_aux[x]+
         18*_aux[OC_MINI(x+1,c_w-1)]-_aux[c_w-1]+64>>7,255);
        if((x<<1|1)<dst_c_w){
          _dst[x<<1|1]=(unsigned char)OC_CLAMPI(0,-3*_aux[x-1]+50*_aux[x]+
           86*_aux[OC_MINI(x+1,c_w-1)]-5*_aux[c_w-1]+64>>7,255);
        }
      }
      _dst+=dst_c_w;
      _aux+=c_w;
    }
  }
}

/*The image is padded with empty chroma components at 4:2:0.
  This costs about 17 bits a frame to code.*/
static void y4m_convert_mono_420jpeg(y4m_input *_y4m,unsigned char *_dst,
 unsigned char *_aux){
  int c_sz;
  _dst+=_y4m->pic_w*_y4m->pic_h;
  c_sz=((_y4m->pic_w+_y4m->dst_c_dec_h-1)/_y4m->dst_c_dec_h)*
   ((_y4m->pic_h+_y4m->dst_c_dec_v-1)/_y4m->dst_c_dec_v);
  memset(_dst,128,c_sz*2);
}

#if 0
/*Right now just 444 to 420.
  Not too hard to generalize.*/
static void y4m_convert_4xxjpeg_42xjpeg(y4m_input *_y4m,unsigned char *_dst,
 unsigned char *_aux){
  unsigned char *tmp;
  int            c_w;
  int            c_h;
  int            pic_sz;
  int            tmp_sz;
  int            c_sz;
  int            pli;
  int            y;
  int            x;
  /*Compute the size of each chroma plane.*/
  c_w=(_y4m->pic_w+_y4m->dst_c_dec_h-1)/_y4m->dst_c_dec_h;
  c_h=(_y4m->pic_h+_y4m->dst_c_dec_v-1)/_y4m->dst_c_dec_v;
  pic_sz=_y4m->pic_w*_y4m->pic_h;
  tmp_sz=c_w*_y4m->pic_h;
  c_sz=c_w*c_h;
  _dst+=pic_sz;
  for(pli=1;pli<3;pli++){
    tmp=_aux+pic_sz;
    /*In reality, the horizontal and vertical steps could be pipelined, for
       less memory consumption and better cache performance, but we do them
       separately for simplicity.*/
    /*First do horizontal filtering (convert to 4:2:2)*/
    /*Filter: [3 -17 78 78 -17 3]/128, derived from a 6-tap Lanczos window.*/
    for(y=0;y<_y4m->pic_h;y++){
      for(x=0;x<OC_MINI(_y4m->pic_w,2);x+=2){
        tmp[x>>1]=OC_CLAMPI(0,64*_aux[0]+78*_aux[OC_MINI(1,_y4m->pic_w-1)]
         -17*_aux[OC_MINI(2,_y4m->pic_w-1)]
         +3*_aux[OC_MINI(3,_y4m->pic_w-1)]+64>>7,255);
      }
      for(;x<_y4m->pic_w-3;x+=2){
        tmp[x>>1]=OC_CLAMPI(0,3*(_aux[x-2]+_aux[x+3])-17*(_aux[x-1]+_aux[x+2])+
         78*(_aux[x]+_aux[x+1])+64>>7,255);
      }
      for(;x<_y4m->pic_w;x+=2){
        tmp[x>>1]=OC_CLAMPI(0,3*(_aux[x-2]+_aux[_y4m->pic_w-1])-
         17*(_aux[x-1]+_aux[OC_MINI(x+2,_y4m->pic_w-1)])+
         78*(_aux[x]+_aux[OC_MINI(x+1,_y4m->pic_w-1)])+64>>7,255);
      }
      tmp+=c_w;
      _aux+=_y4m->pic_w;
    }
    _aux-=pic_sz;
    tmp-=tmp_sz;
    /*Now do the vertical filtering.*/
    for(x=0;x<c_w;x++){
      for(y=0;y<OC_MINI(_y4m->pic_h,2);y+=2){
        _dst[(y>>1)*c_w]=OC_CLAMPI(0,64*tmp[0]
         +78*tmp[OC_MINI(1,_y4m->pic_h-1)*c_w]
         -17*tmp[OC_MINI(2,_y4m->pic_h-1)*c_w]
         +3*tmp[OC_MINI(3,_y4m->pic_h-1)*c_w]+64>>7,255);
      }
      for(;y<_y4m->pic_h-3;y+=2){
        _dst[(y>>1)*c_w]=OC_CLAMPI(0,3*(tmp[(y-2)*c_w]+tmp[(y+3)*c_w])-
         17*(tmp[(y-1)*c_w]+tmp[(y+2)*c_w])+78*(tmp[y*c_w]+tmp[(y+1)*c_w])+
         64>>7,255);
      }
      for(;y<_y4m->pic_h;y+=2){
        _dst[(y>>1)*c_w]=OC_CLAMPI(0,3*(tmp[(y-2)*c_w]
         +tmp[(_y4m->pic_h-1)*c_w])-17*(tmp[(y-1)*c_w]
         +tmp[OC_MINI(y+2,_y4m->pic_h-1)*c_w])
         +78*(tmp[y*c_w]+tmp[OC_MINI(y+1,_y4m->pic_h-1)*c_w])+64>>7,255);
      }
      tmp++;
      _dst++;
    }
    _dst-=c_w;
  }
}
#endif

/*No conversion function needed.*/
static void y4m_convert_null(y4m_input *_y4m,unsigned char *_dst,
 unsigned char *_aux){
}

static int y4m_input_open(y4m_input *_y4m,FILE *_fin,char *_skip,int _nskip){
  char buffer[80];
  int  ret;
  int  i;
  /*Read until newline, or 80 cols, whichever happens first.*/
  for(i=0;i<79;i++){
    if(_nskip>0){
      buffer[i]=*_skip++;
      _nskip--;
    }
    else{
      ret=fread(buffer+i,1,1,_fin);
      if(ret<1)return -1;
    }
    if(buffer[i]=='\n')break;
  }
  /*We skipped too much header data.*/
  if(_nskip>0)return -1;
  if(i==79){
    fprintf(stderr,"Error parsing header; not a YUV2MPEG2 file?\n");
    return -1;
  }
  buffer[i]='\0';
  if(memcmp(buffer,"YUV4MPEG",8)){
    fprintf(stderr,"Incomplete magic for YUV4MPEG file.\n");
    return -1;
  }
  if(buffer[8]!='2'){
    fprintf(stderr,"Incorrect YUV input file version; YUV4MPEG2 required.\n");
  }
  ret=y4m_parse_tags(_y4m,buffer+5);
  if(ret<0){
    fprintf(stderr,"Error parsing YUV4MPEG2 header.\n");
    return ret;
  }
  if(_y4m->interlace!='p'){
    fprintf(stderr,"Input video is interlaced; "
     "Theora only handles progressive scan.\n");
    return -1;
  }
  if(strcmp(_y4m->chroma_type,"420")==0||
   strcmp(_y4m->chroma_type,"420jpeg")==0){
    _y4m->src_c_dec_h=_y4m->dst_c_dec_h=_y4m->src_c_dec_v=_y4m->dst_c_dec_v=2;
    _y4m->dst_buf_read_sz=_y4m->pic_w*_y4m->pic_h
     +2*((_y4m->pic_w+1)/2)*((_y4m->pic_h+1)/2);
    /*Natively supported: no conversion required.*/
    _y4m->aux_buf_sz=_y4m->aux_buf_read_sz=0;
    _y4m->convert=y4m_convert_null;
  }
  else if(strcmp(_y4m->chroma_type,"420mpeg2")==0){
    _y4m->src_c_dec_h=_y4m->dst_c_dec_h=_y4m->src_c_dec_v=_y4m->dst_c_dec_v=2;
    _y4m->dst_buf_read_sz=_y4m->pic_w*_y4m->pic_h;
    /*Chroma filter required: read into the aux buf first.*/
    _y4m->aux_buf_sz=_y4m->aux_buf_read_sz=
     2*((_y4m->pic_w+1)/2)*((_y4m->pic_h+1)/2);
    _y4m->convert=y4m_convert_42xmpeg2_42xjpeg;
  }
  else if(strcmp(_y4m->chroma_type,"420paldv")==0){
    _y4m->src_c_dec_h=_y4m->dst_c_dec_h=_y4m->src_c_dec_v=_y4m->dst_c_dec_v=2;
    _y4m->dst_buf_read_sz=_y4m->pic_w*_y4m->pic_h;
    /*Chroma filter required: read into the aux buf first.
      We need to make two filter passes, so we need some extra space in the
       aux buffer.*/
    _y4m->aux_buf_sz=3*((_y4m->pic_w+1)/2)*((_y4m->pic_h+1)/2);
    _y4m->aux_buf_read_sz=2*((_y4m->pic_w+1)/2)*((_y4m->pic_h+1)/2);
    _y4m->convert=y4m_convert_42xpaldv_42xjpeg;
  }
  else if(strcmp(_y4m->chroma_type,"422")==0){
    _y4m->src_c_dec_h=_y4m->dst_c_dec_h=2;
    _y4m->src_c_dec_v=_y4m->dst_c_dec_v=1;
    _y4m->dst_buf_read_sz=_y4m->pic_w*_y4m->pic_h;
    /*Chroma filter required: read into the aux buf first.*/
    _y4m->aux_buf_sz=_y4m->aux_buf_read_sz=2*((_y4m->pic_w+1)/2)*_y4m->pic_h;
    _y4m->convert=y4m_convert_42xmpeg2_42xjpeg;
  }
  else if(strcmp(_y4m->chroma_type,"411")==0){
    _y4m->src_c_dec_h=4;
    /*We don't want to introduce any additional sub-sampling, so we
       promote 4:1:1 material to 4:2:2, as the closest format Theora can
       handle.*/
    _y4m->dst_c_dec_h=2;
    _y4m->src_c_dec_v=_y4m->dst_c_dec_v=1;
    _y4m->dst_buf_read_sz=_y4m->pic_w*_y4m->pic_h;
    /*Chroma filter required: read into the aux buf first.*/
    _y4m->aux_buf_sz=_y4m->aux_buf_read_sz=2*((_y4m->pic_w+3)/4)*_y4m->pic_h;
    _y4m->convert=y4m_convert_411_422jpeg;
  }
  else if(strcmp(_y4m->chroma_type,"444")==0){
    _y4m->src_c_dec_h=_y4m->dst_c_dec_h=_y4m->src_c_dec_v=_y4m->dst_c_dec_v=1;
    _y4m->dst_buf_read_sz=_y4m->pic_w*_y4m->pic_h*3;
    /*Natively supported: no conversion required.*/
    _y4m->aux_buf_sz=_y4m->aux_buf_read_sz=0;
    _y4m->convert=y4m_convert_null;
  }
  else if(strcmp(_y4m->chroma_type,"444alpha")==0){
    _y4m->src_c_dec_h=_y4m->dst_c_dec_h=_y4m->src_c_dec_v=_y4m->dst_c_dec_v=1;
    _y4m->dst_buf_read_sz=_y4m->pic_w*_y4m->pic_h*3;
    /*Read the extra alpha plane into the aux buf.
      It will be discarded.*/
    _y4m->aux_buf_sz=_y4m->aux_buf_read_sz=_y4m->pic_w*_y4m->pic_h;
    _y4m->convert=y4m_convert_null;
  }
  else if(strcmp(_y4m->chroma_type,"mono")==0){
    _y4m->src_c_dec_h=_y4m->src_c_dec_v=0;
    _y4m->dst_c_dec_h=_y4m->dst_c_dec_v=2;
    _y4m->dst_buf_read_sz=_y4m->pic_w*_y4m->pic_h;
    /*No extra space required, but we need to clear the chroma planes.*/
    _y4m->aux_buf_sz=_y4m->aux_buf_read_sz=0;
    _y4m->convert=y4m_convert_mono_420jpeg;
  }
  else{
    fprintf(stderr,"Unknown chroma sampling type: %s\n",_y4m->chroma_type);
    return -1;
  }
  /*The size of the final frame buffers is always computed from the
     destination chroma decimation type.*/
  _y4m->dst_buf_sz=_y4m->pic_w*_y4m->pic_h
   +2*((_y4m->pic_w+_y4m->dst_c_dec_h-1)/_y4m->dst_c_dec_h)*
   ((_y4m->pic_h+_y4m->dst_c_dec_v-1)/_y4m->dst_c_dec_v);
  /*Scale the picture size up to a multiple of 16.*/
  _y4m->frame_w=_y4m->pic_w+15&~0xF;
  _y4m->frame_h=_y4m->pic_h+15&~0xF;
  /*Force the offsets to be even so that chroma samples line up like we
     expect.*/
  _y4m->pic_x=_y4m->frame_w-_y4m->pic_w>>1&~1;
  _y4m->pic_y=_y4m->frame_h-_y4m->pic_h>>1&~1;
  _y4m->dst_buf=(unsigned char *)malloc(_y4m->dst_buf_sz);
  _y4m->aux_buf=(unsigned char *)malloc(_y4m->aux_buf_sz);
  return 0;
}

static void y4m_input_get_info(y4m_input *_y4m,th_info *_ti){
  _ti->frame_width=_y4m->frame_w;
  _ti->frame_height=_y4m->frame_h;
  _ti->pic_width=_y4m->pic_w;
  _ti->pic_height=_y4m->pic_h;
  _ti->pic_x=_y4m->pic_x;
  _ti->pic_y=_y4m->pic_y;
  _ti->fps_numerator=_y4m->fps_n;
  _ti->fps_denominator=_y4m->fps_d;
  _ti->aspect_numerator=_y4m->par_n;
  _ti->aspect_denominator=_y4m->par_d;
  _ti->pixel_fmt=_y4m->dst_c_dec_h==2?
   (_y4m->dst_c_dec_v==2?TH_PF_420:TH_PF_422):TH_PF_444;
}

static int y4m_input_fetch_frame(y4m_input *_y4m,FILE *_fin,
 th_ycbcr_buffer _ycbcr){
  char frame[6];
  int  pic_sz;
  int  frame_c_w;
  int  frame_c_h;
  int  c_w;
  int  c_h;
  int  c_sz;
  int  ret;
  pic_sz=_y4m->pic_w*_y4m->pic_h;
  frame_c_w=_y4m->frame_w/_y4m->dst_c_dec_h;
  frame_c_h=_y4m->frame_h/_y4m->dst_c_dec_v;
  c_w=(_y4m->pic_w+_y4m->dst_c_dec_h-1)/_y4m->dst_c_dec_h;
  c_h=(_y4m->pic_h+_y4m->dst_c_dec_v-1)/_y4m->dst_c_dec_v;
  c_sz=c_w*c_h;
  /*Read and skip the frame header.*/
  ret=fread(frame,1,6,_fin);
  if(ret<6)return 0;
  if(memcmp(frame,"FRAME",5)){
    fprintf(stderr,"Loss of framing in YUV input data\n");
    exit(1);
  }
  if(frame[5]!='\n'){
    char c;
    int  j;
    for(j=0;j<79&&fread(&c,1,1,_fin)&&c!='\n';j++);
    if(j==79){
      fprintf(stderr,"Error parsing YUV frame header\n");
      return -1;
    }
  }
  /*Read the frame data that needs no conversion.*/
  if(fread(_y4m->dst_buf,1,_y4m->dst_buf_read_sz,_fin)!=_y4m->dst_buf_read_sz){
    fprintf(stderr,"Error reading YUV frame data.\n");
    return -1;
  }
  /*Read the frame data that does need conversion.*/
  if(fread(_y4m->aux_buf,1,_y4m->aux_buf_read_sz,_fin)!=_y4m->aux_buf_read_sz){
    fprintf(stderr,"Error reading YUV frame data.\n");
    return -1;
  }
  /*Now convert the just read frame.*/
  (*_y4m->convert)(_y4m,_y4m->dst_buf,_y4m->aux_buf);
  /*Fill in the frame buffer pointers.*/
  _ycbcr[0].width=_y4m->frame_w;
  _ycbcr[0].height=_y4m->frame_h;
  _ycbcr[0].stride=_y4m->pic_w;
  _ycbcr[0].data=_y4m->dst_buf-_y4m->pic_x-_y4m->pic_y*_y4m->pic_w;
  _ycbcr[1].width=frame_c_w;
  _ycbcr[1].height=frame_c_h;
  _ycbcr[1].stride=c_w;
  _ycbcr[1].data=_y4m->dst_buf+pic_sz-(_y4m->pic_x/_y4m->dst_c_dec_h)-
   (_y4m->pic_y/_y4m->dst_c_dec_v)*c_w;
  _ycbcr[2].width=frame_c_w;
  _ycbcr[2].height=frame_c_h;
  _ycbcr[2].stride=c_w;
  _ycbcr[2].data=_ycbcr[1].data+c_sz;
  return 1;
}

static void y4m_input_close(y4m_input *_y4m){
  free(_y4m->dst_buf);
  free(_y4m->aux_buf);
}



typedef struct th_input th_input;

struct th_input{
  ogg_sync_state    oy;
  int               theora_p;
  ogg_stream_state  to;
  th_info           ti;
  th_comment        tc;
  th_dec_ctx       *td;
};



/*Grab some more compressed bitstream and sync it for page extraction.*/
static int th_input_buffer_data(th_input *_th,FILE *_fin){
  char *buffer;
  int bytes;
  buffer=ogg_sync_buffer(&_th->oy,4096);
  bytes=fread(buffer,1,4096,_fin);
  ogg_sync_wrote(&_th->oy,bytes);
  return bytes;
}

/*Push a page into the appropriate steam.
  This can be done blindly; a stream won't accept a page that doesn't belong to
   it.*/
static void th_input_queue_page(th_input *_th,ogg_page *_og){
  if(_th->theora_p)ogg_stream_pagein(&_th->to,_og);
}

static int th_input_open_impl(th_input *_th,th_setup_info **_ts,FILE *_fin,
 char *_sig,int _nsig){
  ogg_packet op;
  ogg_page   og;
  int        nheaders_left;
  int        done_headers;
  ogg_sync_init(&_th->oy);
  th_info_init(&_th->ti);
  th_comment_init(&_th->tc);
  *_ts=NULL;
  /*Buffer any initial data read for file ID.*/
  if(_nsig>0){
    char *buffer;
    buffer=ogg_sync_buffer(&_th->oy,_nsig);
    memcpy(buffer,_sig,_nsig);
    ogg_sync_wrote(&_th->oy,_nsig);
  }
  _th->theora_p=0;
  nheaders_left=0;
  for(done_headers=0;!done_headers;){
    if(th_input_buffer_data(_th,_fin)==0)break;
    while(ogg_sync_pageout(&_th->oy,&og)>0){
      ogg_stream_state test;
      /*Is this a mandated initial header?
        If not, stop parsing.*/
      if(!ogg_page_bos(&og)){
        /*Don't leak the page; get it into the appropriate stream.*/
        th_input_queue_page(_th,&og);
        done_headers=1;
        break;
      }
      ogg_stream_init(&test,ogg_page_serialno(&og));
      ogg_stream_pagein(&test,&og);
      ogg_stream_packetpeek(&test,&op);
      /*Identify the codec: try Theora.*/
      if(!_th->theora_p){
        nheaders_left=th_decode_headerin(&_th->ti,&_th->tc,_ts,&op);
        if(nheaders_left>=0){
          /*It is Theora.*/
          memcpy(&_th->to,&test,sizeof(test));
          _th->theora_p=1;
          /*Advance past the successfully processed header.*/
          if(nheaders_left>0)ogg_stream_packetout(&_th->to,NULL);
          continue;
        }
      }
      /*Whatever it is, we don't care about it.*/
      ogg_stream_clear(&test);
    }
  }
  /*We're expecting more header packets.*/
  while(_th->theora_p&&nheaders_left>0){
    int ret;
    while(nheaders_left>0){
      ret=ogg_stream_packetpeek(&_th->to,&op);
      if(ret==0)break;
      if(ret<0)continue;
      nheaders_left=th_decode_headerin(&_th->ti,&_th->tc,_ts,&op);
      if(nheaders_left<0){
        fprintf(stderr,"Error parsing Theora stream headers; "
         "corrupt stream?\n");
        return -1;
      }
      /*Advance past the successfully processed header.*/
      else if(nheaders_left>0)ogg_stream_packetout(&_th->to,NULL);
      _th->theora_p++;
    }
    /*Stop now so we don't fail if there aren't enough pages in a short
       stream.*/
    if(!(_th->theora_p&&nheaders_left>0))break;
    /*The header pages/packets will arrive before anything else we care
       about, or the stream is not obeying spec.*/
    if(ogg_sync_pageout(&_th->oy,&og)>0)th_input_queue_page(_th,&og);
    /*We need more data.*/
    else if(th_input_buffer_data(_th,_fin)==0){
      fprintf(stderr,"End of file while searching for codec headers.\n");
      return -1;
    }
  }
  /*And now we have it all.
    Initialize the decoder.*/
  if(_th->theora_p){
    _th->td=th_decode_alloc(&_th->ti,*_ts);
    if(_th->td!=NULL){
      fprintf(stderr,"Ogg logical stream %lx is Theora %ix%i %.02f fps video.\n"
       "Encoded frame content is %ix%i with %ix%i offset.\n",
       _th->to.serialno,_th->ti.frame_width,_th->ti.frame_height,
       (double)_th->ti.fps_numerator/_th->ti.fps_denominator,
       _th->ti.pic_width,_th->ti.pic_height,_th->ti.pic_x,_th->ti.pic_y);
      return 1;
    }
  }
  return -1;
}

static void th_input_close(th_input *_th){
  if(_th->theora_p){
    ogg_stream_clear(&_th->to);
    th_decode_free(_th->td);
  }
  th_comment_clear(&_th->tc);
  th_info_clear(&_th->ti);
  ogg_sync_clear(&_th->oy);
}

static int th_input_open(th_input *_th,FILE *_fin,char *_sig,int _nsig){
  th_input       th;
  th_setup_info *ts;
  int            ret;
  ret=th_input_open_impl(&th,&ts,_fin,_sig,_nsig);
  th_setup_free(ts);
  /*Clean up on failure.*/
  if(ret<0)th_input_close(&th);
  else memcpy(_th,&th,sizeof(th));
  return ret;
}

static void th_input_get_info(th_input *_th,th_info *_ti){
  memcpy(_ti,&_th->ti,sizeof(*_ti));
}

static int th_input_fetch_frame(th_input *_th,FILE *_fin,
 th_ycbcr_buffer _ycbcr){
  for(;;){
    ogg_page   og;
    ogg_packet op;
    if(ogg_stream_packetout(&_th->to,&op)>0){
      if(th_decode_packetin(_th->td,&op,NULL)>=0){
        th_decode_ycbcr_out(_th->td,_ycbcr);
        if(!summary_only&&show_frame_type){
          printf("%c",th_packet_iskeyframe(&op)?'K':'D');
          if(op.bytes>0)printf("%02i ",op.packet[0]&0x3F);
          else printf("-- ");
        }
        return 1;
      }
      else return -1;
    }
    while(ogg_sync_pageout(&_th->oy,&og)<=0){
      if(th_input_buffer_data(_th,_fin)==0)return feof(_fin)?0:-1;
    }
    th_input_queue_page(_th,&og);
  }
}



typedef struct video_input video_input;
typedef void (*video_input_get_info_func)(void *_ctx,th_info *_ti);
typedef int (*video_input_fetch_frame_func)(void *_ctx,FILE *_fin,
 th_ycbcr_buffer _ycbcr);
typedef void (*video_input_close_func)(void *_ctx);

struct video_input{
  FILE                         *fin;
  video_input_get_info_func     get_info;
  video_input_fetch_frame_func  fetch_frame;
  video_input_close_func        close;
  union{
    y4m_input y4m;
    th_input  th;
  }ctx;
};

static int video_input_open(video_input *_vid,FILE *_fin){
  char buffer[4];
  int  ret;
  /* look for magic */
  ret=fread(buffer,1,4,_fin);
  if(ret<4)fprintf(stderr,"EOF determining file type of file.\n");
  else{
    if(!memcmp(buffer,"YUV4",4)){
      if(y4m_input_open(&_vid->ctx.y4m,_fin,buffer,4)>=0){
        /*fprintf(stderr,"Original %s is %dx%d %.02f fps %s video.\n",
         f,_y4m->pic_w,_y4m->pic_h,(double)_y4m->fps_n/_y4m->fps_d,_y4m->chroma_type);*/
        _vid->fin=_fin;
        _vid->get_info=(video_input_get_info_func)y4m_input_get_info;
        _vid->fetch_frame=(video_input_fetch_frame_func)y4m_input_fetch_frame;
        _vid->close=(video_input_close_func)y4m_input_close;
        return 0;
      }
    }
    else if(!memcmp(buffer,"OggS",4)){
      if(th_input_open(&_vid->ctx.th,_fin,buffer,4)>=0){
        _vid->fin=_fin;
        _vid->get_info=(video_input_get_info_func)th_input_get_info;
        _vid->fetch_frame=(video_input_fetch_frame_func)th_input_fetch_frame;
        _vid->close=(video_input_close_func)th_input_close;
        return 0;
      }
    }
    else fprintf(stderr,"Unknown file type.\n");
  }
  return -1;
}

static void video_input_get_info(video_input *_vid,th_info *_ti){
  (*_vid->get_info)(&_vid->ctx,_ti);
}

static int video_input_fetch_frame(video_input *_vid,th_ycbcr_buffer _ycbcr){
  return (*_vid->fetch_frame)(&_vid->ctx,_vid->fin,_ycbcr);
}

static void video_input_close(video_input *_vid){
  (*_vid->close)(&_vid->ctx);
  fclose(_vid->fin);
}



static void usage(char *_argv[]){
  fprintf(stderr,"Usage: %s [options] <video1> <video2>\n"
   "    <video1> and <video1> may be either YUV4MPEG or Ogg Theora files.\n\n"
   "    Options:\n\n"
   "      -f --frame-type Show frame type and QI value for each Theora frame.\n"
   "      -s --summary    Only output the summary line.\n"
   "      -y --luma-only  Only output values for the luma channel.\n",_argv[0]);
}

int main(int _argc,char *_argv[]){
  video_input  vid1;
  th_info      ti1;
  video_input  vid2;
  th_info      ti2;
  ogg_int64_t  gsqerr;
  ogg_int64_t  gnpixels;
  ogg_int64_t  gplsqerr[3];
  ogg_int64_t  gplnpixels[3];
  int          frameno;
  FILE        *fin;
  int          long_option_index;
  int          c;
#ifdef _WIN32
  /*We need to set stdin/stdout to binary mode on windows.
    Beware the evil ifdef.
    We avoid these where we can, but this one we cannot.
    Don't add any more, you'll probably go to hell if you do.*/
  _setmode(_fileno(stdin),_O_BINARY);
#endif
  /*Process option arguments.*/
  while((c=getopt_long(_argc,_argv,optstring,options,&long_option_index))!=EOF){
    switch(c){
      case 'f':show_frame_type=1;break;
      case 's':summary_only=1;break;
      case 'y':luma_only=1;break;
      default:usage(_argv);break;
    }
  }
  if(optind+2!=_argc){
    usage(_argv);
    exit(1);
  }
  fin=strcmp(_argv[optind],"-")==0?stdin:fopen(_argv[optind],"rb");
  if(fin==NULL){
    fprintf(stderr,"Unable to open '%s' for extraction.\n",_argv[optind]);
    exit(1);
  }
  fprintf(stderr,"Opening %s...\n",_argv[optind]);
  if(video_input_open(&vid1,fin)<0)exit(1);
  video_input_get_info(&vid1,&ti1);
  fin=strcmp(_argv[optind+1],"-")==0?stdin:fopen(_argv[optind+1],"rb");
  if(fin==NULL){
    fprintf(stderr,"Unable to open '%s' for extraction.\n",_argv[optind+1]);
    exit(1);
  }
  fprintf(stderr,"Opening %s...\n",_argv[optind+1]);
  if(video_input_open(&vid2,fin)<0)exit(1);
  video_input_get_info(&vid2,&ti2);
  /*Check to make sure these videos are compatible.*/
  if(ti1.pic_width!=ti2.pic_width||ti1.pic_height!=ti2.pic_height){
    fprintf(stderr,"Video resolution does not match.\n");
    exit(1);
  }
  if(ti1.pixel_fmt!=ti2.pixel_fmt){
    fprintf(stderr,"Pixel formats do not match.\n");
    exit(1);
  }
  if((ti1.pic_x&!(ti1.pixel_fmt&1))!=(ti2.pic_x&!(ti2.pixel_fmt&1))||
   (ti1.pic_y&!(ti1.pixel_fmt&2))!=(ti2.pic_y&!(ti2.pixel_fmt&2))){
    fprintf(stderr,"Chroma subsampling offsets do not match.\n");
    exit(1);
  }
  if(ti1.fps_numerator*(ogg_int64_t)ti2.fps_denominator!=
   ti2.fps_numerator*(ogg_int64_t)ti1.fps_denominator){
    fprintf(stderr,"Warning: framerates do not match.\n");
  }
  if(ti1.aspect_numerator*(ogg_int64_t)ti2.aspect_denominator!=
   ti2.aspect_numerator*(ogg_int64_t)ti1.aspect_denominator){
    fprintf(stderr,"Warning: aspect ratios do not match.\n");
  }
  gsqerr=gplsqerr[0]=gplsqerr[1]=gplsqerr[2]=0;
  gnpixels=gplnpixels[0]=gplnpixels[1]=gplnpixels[2]=0;
  for(frameno=0;;frameno++){
    th_ycbcr_buffer f1;
    th_ycbcr_buffer f2;
    ogg_int64_t     plsqerr[3];
    long            plnpixels[3];
    ogg_int64_t     sqerr;
    long            npixels;
    int             ret1;
    int             ret2;
    int             pli;
    ret1=video_input_fetch_frame(&vid1,f1);
    ret2=video_input_fetch_frame(&vid2,f2);
    if(ret1==0&&ret2==0)break;
    else if(ret1<0||ret2<0)break;
    else if(ret1==0){
      fprintf(stderr,"%s ended before %s.\n",
       _argv[optind],_argv[optind+1]);
      break;
    }
    else if(ret2==0){
      fprintf(stderr,"%s ended before %s.\n",
       _argv[optind+1],_argv[optind]);
      break;
    }
    /*Okay, we got one frame from each.*/
    sqerr=0;
    npixels=0;
    for(pli=0;pli<3;pli++){
      int xdec;
      int ydec;
      int y1;
      int y2;
      xdec=pli&&!(ti1.pixel_fmt&1);
      ydec=pli&&!(ti1.pixel_fmt&2);
      plsqerr[pli]=0;
      plnpixels[pli]=0;
      for(y1=ti1.pic_y>>ydec,y2=ti2.pic_y>>ydec;
       y1<ti1.pic_y+ti1.pic_height+ydec>>ydec;y1++,y2++){
        int x1;
        int x2;
        for(x1=ti1.pic_x>>xdec,x2=ti2.pic_x>>xdec;
         x1<ti1.pic_x+ti1.pic_width+xdec>>xdec;x1++,x2++){
          int d;
          d=*(f1[pli].data+y1*f1[pli].stride+x1)-
           *(f2[pli].data+y2*f2[pli].stride+x2);
          plsqerr[pli]+=d*d;
          plnpixels[pli]++;
        }
      }
      sqerr+=plsqerr[pli];
      gplsqerr[pli]+=plsqerr[pli];
      npixels+=plnpixels[pli];
      gplnpixels[pli]+=plnpixels[pli];
    }
    if(!summary_only){
      if(!luma_only){
        printf("%08i: %-7lG  (Y': %-7lG  Cb: %-7lG  Cr: %-7lG)\n",frameno,
         10*(log10(255*255)+log10(npixels)-log10(sqerr)),
         10*(log10(255*255)+log10(plnpixels[0])-log10(plsqerr[0])),
         10*(log10(255*255)+log10(plnpixels[1])-log10(plsqerr[1])),
         10*(log10(255*255)+log10(plnpixels[2])-log10(plsqerr[2])));
      }
      else{
        printf("%08i: %-7lG\n",frameno,
         10*(log10(255*255)+log10(plnpixels[0])-log10(plsqerr[0])));
      }
    }
    gsqerr+=sqerr;
    gnpixels+=npixels;
  }
  if(!luma_only){
    printf("Total: %-7lG  (Y': %-7lG  Cb: %-7lG  Cr: %-7lG)\n",
     10*(log10(255*255)+log10(gnpixels)-log10(gsqerr)),
     10*(log10(255*255)+log10(gplnpixels[0])-log10(gplsqerr[0])),
     10*(log10(255*255)+log10(gplnpixels[1])-log10(gplsqerr[1])),
     10*(log10(255*255)+log10(gplnpixels[2])-log10(gplsqerr[2])));
  }
  else{
    printf("Total: %-7lG\n",
     10*(log10(255*255)+log10(gplnpixels[0])-log10(gplsqerr[0])));
  }
  video_input_close(&vid1);
  video_input_close(&vid2);
  return 0;
}
