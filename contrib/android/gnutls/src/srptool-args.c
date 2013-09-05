/*   -*- buffer-read-only: t -*- vi: set ro:
 *  
 *  DO NOT EDIT THIS FILE   (srptool-args.c)
 *  
 *  It has been AutoGen-ed  March 22, 2013 at 07:34:46 PM by AutoGen 5.16
 *  From the definitions    srptool-args.def
 *  and the template file   options
 *
 * Generated from AutoOpts 36:4:11 templates.
 *
 *  AutoOpts is a copyrighted work.  This source file is not encumbered
 *  by AutoOpts licensing, but is provided under the licensing terms chosen
 *  by the srptool author or copyright holder.  AutoOpts is
 *  licensed under the terms of the LGPL.  The redistributable library
 *  (``libopts'') is licensed under the terms of either the LGPL or, at the
 *  users discretion, the BSD license.  See the AutoOpts and/or libopts sources
 *  for details.
 *
 * The srptool program is copyrighted and licensed
 * under the following terms:
 *
 *  Copyright (C) 2000-2012 Free Software Foundation, all rights reserved.
 *  This is free software. It is licensed for use, modification and
 *  redistribution under the terms of the
 *  GNU General Public License, version 3 or later
 *      <http://gnu.org/licenses/gpl.html>
 *
 *  srptool is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the
 *  Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *  
 *  srptool is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *  See the GNU General Public License for more details.
 *  
 *  You should have received a copy of the GNU General Public License along
 *  with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __doxygen__
#define OPTION_CODE_COMPILE 1
#include "srptool-args.h"
#include <sys/types.h>

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#ifdef  __cplusplus
extern "C" {
#endif
extern FILE * option_usage_fp;

/* TRANSLATORS: choose the translation for option names wisely because you
                cannot ever change your mind. */
#define zCopyright      (srptool_opt_strs+0)
#define zLicenseDescrip (srptool_opt_strs+275)


#ifndef NULL
#  define NULL 0
#endif

/*
 *  srptool option static const strings
 */
static char const srptool_opt_strs[2040] =
/*     0 */ "srptool 3.1.10\n"
            "Copyright (C) 2000-2012 Free Software Foundation, all rights reserved.\n"
            "This is free software. It is licensed for use, modification and\n"
            "redistribution under the terms of the\n"
            "GNU General Public License, version 3 or later\n"
            "    <http://gnu.org/licenses/gpl.html>\n\0"
/*   275 */ "srptool is free software: you can redistribute it and/or modify it under\n"
            "the terms of the GNU General Public License as published by the Free\n"
            "Software Foundation, either version 3 of the License, or (at your option)\n"
            "any later version.\n\n"
            "srptool is distributed in the hope that it will be useful, but WITHOUT ANY\n"
            "WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS\n"
            "FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more\n"
            "details.\n\n"
            "You should have received a copy of the GNU General Public License along\n"
            "with this program.  If not, see <http://www.gnu.org/licenses/>.\n\0"
/*   878 */ "Enable debugging.\0"
/*   896 */ "DEBUG\0"
/*   902 */ "debug\0"
/*   908 */ "specify the index of the group parameters in tpasswd.conf to use.\0"
/*   974 */ "INDEX\0"
/*   980 */ "index\0"
/*   986 */ "specify a username\0"
/*  1005 */ "USERNAME\0"
/*  1014 */ "username\0"
/*  1023 */ "specify a password file.\0"
/*  1048 */ "PASSWD\0"
/*  1055 */ "passwd\0"
/*  1062 */ "specify salt size.\0"
/*  1081 */ "SALT\0"
/*  1086 */ "salt\0"
/*  1091 */ "just verify the password.\0"
/*  1117 */ "VERIFY\0"
/*  1124 */ "verify\0"
/*  1131 */ "specify a password conf file.\0"
/*  1161 */ "PASSWD_CONF\0"
/*  1173 */ "passwd-conf\0"
/*  1185 */ "Generate a password configuration file.\0"
/*  1225 */ "CREATE_CONF\0"
/*  1237 */ "create-conf\0"
/*  1249 */ "Display extended usage information and exit\0"
/*  1293 */ "help\0"
/*  1298 */ "Extended usage information passed thru pager\0"
/*  1343 */ "more-help\0"
/*  1353 */ "Output version information and exit\0"
/*  1389 */ "version\0"
/*  1397 */ "SRPTOOL\0"
/*  1405 */ "srptool - GnuTLS SRP tool - Ver. 3.1.10\n"
            "USAGE:  %s [ -<flag> [<val>] | --<name>[{=| }<val>] ]...\n\0"
/*  1503 */ "bug-gnutls@gnu.org\0"
/*  1522 */ "\n\n\0"
/*  1525 */ "\n"
            "Simple program that emulates the programs in the Stanford SRP (Secure\n"
            "Remote Password) libraries using GnuTLS.  It is intended for use in places\n"
            "where you don't expect SRP authentication to be the used for system users.\n\n"
            "In brief, to use SRP you need to create two files.  These are the password\n"
            "file that holds the users and the verifiers associated with them and the\n"
            "configuration file to hold the group parameters (called tpasswd.conf).\n\0"
/*  1967 */ "srptool 3.1.10\0"
/*  1982 */ "srptool [options]\n"
            "srptool --help for usage instructions.\n";

/*
 *  debug option description:
 */
#define DEBUG_DESC      (srptool_opt_strs+878)
#define DEBUG_NAME      (srptool_opt_strs+896)
#define DEBUG_name      (srptool_opt_strs+902)
#define DEBUG_FLAGS     (OPTST_DISABLED \
        | OPTST_SET_ARGTYPE(OPARG_TYPE_NUMERIC))

/*
 *  index option description:
 */
#define INDEX_DESC      (srptool_opt_strs+908)
#define INDEX_NAME      (srptool_opt_strs+974)
#define INDEX_name      (srptool_opt_strs+980)
#define INDEX_FLAGS     (OPTST_DISABLED)

/*
 *  username option description:
 */
#define USERNAME_DESC      (srptool_opt_strs+986)
#define USERNAME_NAME      (srptool_opt_strs+1005)
#define USERNAME_name      (srptool_opt_strs+1014)
#define USERNAME_FLAGS     (OPTST_DISABLED \
        | OPTST_SET_ARGTYPE(OPARG_TYPE_STRING))

/*
 *  passwd option description:
 */
#define PASSWD_DESC      (srptool_opt_strs+1023)
#define PASSWD_NAME      (srptool_opt_strs+1048)
#define PASSWD_name      (srptool_opt_strs+1055)
#define PASSWD_FLAGS     (OPTST_DISABLED \
        | OPTST_SET_ARGTYPE(OPARG_TYPE_STRING))

/*
 *  salt option description:
 */
#define SALT_DESC      (srptool_opt_strs+1062)
#define SALT_NAME      (srptool_opt_strs+1081)
#define SALT_name      (srptool_opt_strs+1086)
#define SALT_FLAGS     (OPTST_DISABLED \
        | OPTST_SET_ARGTYPE(OPARG_TYPE_NUMERIC))

/*
 *  verify option description:
 */
#define VERIFY_DESC      (srptool_opt_strs+1091)
#define VERIFY_NAME      (srptool_opt_strs+1117)
#define VERIFY_name      (srptool_opt_strs+1124)
#define VERIFY_FLAGS     (OPTST_DISABLED)

/*
 *  passwd-conf option description:
 */
#define PASSWD_CONF_DESC      (srptool_opt_strs+1131)
#define PASSWD_CONF_NAME      (srptool_opt_strs+1161)
#define PASSWD_CONF_name      (srptool_opt_strs+1173)
#define PASSWD_CONF_FLAGS     (OPTST_DISABLED \
        | OPTST_SET_ARGTYPE(OPARG_TYPE_STRING))

/*
 *  create-conf option description:
 */
#define CREATE_CONF_DESC      (srptool_opt_strs+1185)
#define CREATE_CONF_NAME      (srptool_opt_strs+1225)
#define CREATE_CONF_name      (srptool_opt_strs+1237)
#define CREATE_CONF_FLAGS     (OPTST_DISABLED \
        | OPTST_SET_ARGTYPE(OPARG_TYPE_STRING))

/*
 *  Help/More_Help/Version option descriptions:
 */
#define HELP_DESC       (srptool_opt_strs+1249)
#define HELP_name       (srptool_opt_strs+1293)
#ifdef HAVE_WORKING_FORK
#define MORE_HELP_DESC  (srptool_opt_strs+1298)
#define MORE_HELP_name  (srptool_opt_strs+1343)
#define MORE_HELP_FLAGS (OPTST_IMM | OPTST_NO_INIT)
#else
#define MORE_HELP_DESC  NULL
#define MORE_HELP_name  NULL
#define MORE_HELP_FLAGS (OPTST_OMITTED | OPTST_NO_INIT)
#endif
#ifdef NO_OPTIONAL_OPT_ARGS
#  define VER_FLAGS     (OPTST_IMM | OPTST_NO_INIT)
#else
#  define VER_FLAGS     (OPTST_SET_ARGTYPE(OPARG_TYPE_STRING) | \
                         OPTST_ARG_OPTIONAL | OPTST_IMM | OPTST_NO_INIT)
#endif
#define VER_DESC        (srptool_opt_strs+1353)
#define VER_name        (srptool_opt_strs+1389)
/*
 *  Declare option callback procedures
 */
extern tOptProc
    optionBooleanVal,   optionNestedVal,    optionNumericVal,
    optionPagedUsage,   optionPrintVersion, optionResetOpt,
    optionStackArg,     optionTimeDate,     optionTimeVal,
    optionUnstackArg,   optionVendorOption;
static tOptProc
    doOptDebug, doUsageOpt;
#define VER_PROC        optionPrintVersion

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/**
 *  Define the srptool Option Descriptions.
 * This is an array of OPTION_CT entries, one for each
 * option that the srptool program responds to.
 */
static tOptDesc optDesc[OPTION_CT] = {
  {  /* entry idx, value */ 0, VALUE_OPT_DEBUG,
     /* equiv idx, value */ 0, VALUE_OPT_DEBUG,
     /* equivalenced to  */ NO_EQUIVALENT,
     /* min, max, act ct */ 0, 1, 0,
     /* opt state flags  */ DEBUG_FLAGS, 0,
     /* last opt argumnt */ { NULL }, /* --debug */
     /* arg list/cookie  */ NULL,
     /* must/cannot opts */ NULL, NULL,
     /* option proc      */ doOptDebug,
     /* desc, NAME, name */ DEBUG_DESC, DEBUG_NAME, DEBUG_name,
     /* disablement strs */ NULL, NULL },

  {  /* entry idx, value */ 1, VALUE_OPT_INDEX,
     /* equiv idx, value */ 1, VALUE_OPT_INDEX,
     /* equivalenced to  */ NO_EQUIVALENT,
     /* min, max, act ct */ 0, 1, 0,
     /* opt state flags  */ INDEX_FLAGS, 0,
     /* last opt argumnt */ { NULL }, /* --index */
     /* arg list/cookie  */ NULL,
     /* must/cannot opts */ NULL, NULL,
     /* option proc      */ NULL,
     /* desc, NAME, name */ INDEX_DESC, INDEX_NAME, INDEX_name,
     /* disablement strs */ NULL, NULL },

  {  /* entry idx, value */ 2, VALUE_OPT_USERNAME,
     /* equiv idx, value */ 2, VALUE_OPT_USERNAME,
     /* equivalenced to  */ NO_EQUIVALENT,
     /* min, max, act ct */ 0, 1, 0,
     /* opt state flags  */ USERNAME_FLAGS, 0,
     /* last opt argumnt */ { NULL }, /* --username */
     /* arg list/cookie  */ NULL,
     /* must/cannot opts */ NULL, NULL,
     /* option proc      */ NULL,
     /* desc, NAME, name */ USERNAME_DESC, USERNAME_NAME, USERNAME_name,
     /* disablement strs */ NULL, NULL },

  {  /* entry idx, value */ 3, VALUE_OPT_PASSWD,
     /* equiv idx, value */ 3, VALUE_OPT_PASSWD,
     /* equivalenced to  */ NO_EQUIVALENT,
     /* min, max, act ct */ 0, 1, 0,
     /* opt state flags  */ PASSWD_FLAGS, 0,
     /* last opt argumnt */ { NULL }, /* --passwd */
     /* arg list/cookie  */ NULL,
     /* must/cannot opts */ NULL, NULL,
     /* option proc      */ NULL,
     /* desc, NAME, name */ PASSWD_DESC, PASSWD_NAME, PASSWD_name,
     /* disablement strs */ NULL, NULL },

  {  /* entry idx, value */ 4, VALUE_OPT_SALT,
     /* equiv idx, value */ 4, VALUE_OPT_SALT,
     /* equivalenced to  */ NO_EQUIVALENT,
     /* min, max, act ct */ 0, 1, 0,
     /* opt state flags  */ SALT_FLAGS, 0,
     /* last opt argumnt */ { NULL }, /* --salt */
     /* arg list/cookie  */ NULL,
     /* must/cannot opts */ NULL, NULL,
     /* option proc      */ optionNumericVal,
     /* desc, NAME, name */ SALT_DESC, SALT_NAME, SALT_name,
     /* disablement strs */ NULL, NULL },

  {  /* entry idx, value */ 5, VALUE_OPT_VERIFY,
     /* equiv idx, value */ 5, VALUE_OPT_VERIFY,
     /* equivalenced to  */ NO_EQUIVALENT,
     /* min, max, act ct */ 0, 1, 0,
     /* opt state flags  */ VERIFY_FLAGS, 0,
     /* last opt argumnt */ { NULL }, /* --verify */
     /* arg list/cookie  */ NULL,
     /* must/cannot opts */ NULL, NULL,
     /* option proc      */ NULL,
     /* desc, NAME, name */ VERIFY_DESC, VERIFY_NAME, VERIFY_name,
     /* disablement strs */ NULL, NULL },

  {  /* entry idx, value */ 6, VALUE_OPT_PASSWD_CONF,
     /* equiv idx, value */ 6, VALUE_OPT_PASSWD_CONF,
     /* equivalenced to  */ NO_EQUIVALENT,
     /* min, max, act ct */ 0, 1, 0,
     /* opt state flags  */ PASSWD_CONF_FLAGS, 0,
     /* last opt argumnt */ { NULL }, /* --passwd-conf */
     /* arg list/cookie  */ NULL,
     /* must/cannot opts */ NULL, NULL,
     /* option proc      */ NULL,
     /* desc, NAME, name */ PASSWD_CONF_DESC, PASSWD_CONF_NAME, PASSWD_CONF_name,
     /* disablement strs */ NULL, NULL },

  {  /* entry idx, value */ 7, VALUE_OPT_CREATE_CONF,
     /* equiv idx, value */ 7, VALUE_OPT_CREATE_CONF,
     /* equivalenced to  */ NO_EQUIVALENT,
     /* min, max, act ct */ 0, 1, 0,
     /* opt state flags  */ CREATE_CONF_FLAGS, 0,
     /* last opt argumnt */ { NULL }, /* --create-conf */
     /* arg list/cookie  */ NULL,
     /* must/cannot opts */ NULL, NULL,
     /* option proc      */ NULL,
     /* desc, NAME, name */ CREATE_CONF_DESC, CREATE_CONF_NAME, CREATE_CONF_name,
     /* disablement strs */ NULL, NULL },

  {  /* entry idx, value */ INDEX_OPT_VERSION, VALUE_OPT_VERSION,
     /* equiv idx value  */ NO_EQUIVALENT, VALUE_OPT_VERSION,
     /* equivalenced to  */ NO_EQUIVALENT,
     /* min, max, act ct */ 0, 1, 0,
     /* opt state flags  */ VER_FLAGS, 0,
     /* last opt argumnt */ { NULL },
     /* arg list/cookie  */ NULL,
     /* must/cannot opts */ NULL, NULL,
     /* option proc      */ VER_PROC,
     /* desc, NAME, name */ VER_DESC, NULL, VER_name,
     /* disablement strs */ NULL, NULL },



  {  /* entry idx, value */ INDEX_OPT_HELP, VALUE_OPT_HELP,
     /* equiv idx value  */ NO_EQUIVALENT, VALUE_OPT_HELP,
     /* equivalenced to  */ NO_EQUIVALENT,
     /* min, max, act ct */ 0, 1, 0,
     /* opt state flags  */ OPTST_IMM | OPTST_NO_INIT, 0,
     /* last opt argumnt */ { NULL },
     /* arg list/cookie  */ NULL,
     /* must/cannot opts */ NULL, NULL,
     /* option proc      */ doUsageOpt,
     /* desc, NAME, name */ HELP_DESC, NULL, HELP_name,
     /* disablement strs */ NULL, NULL },

  {  /* entry idx, value */ INDEX_OPT_MORE_HELP, VALUE_OPT_MORE_HELP,
     /* equiv idx value  */ NO_EQUIVALENT, VALUE_OPT_MORE_HELP,
     /* equivalenced to  */ NO_EQUIVALENT,
     /* min, max, act ct */ 0, 1, 0,
     /* opt state flags  */ MORE_HELP_FLAGS, 0,
     /* last opt argumnt */ { NULL },
     /* arg list/cookie  */ NULL,
     /* must/cannot opts */ NULL,  NULL,
     /* option proc      */ optionPagedUsage,
     /* desc, NAME, name */ MORE_HELP_DESC, NULL, MORE_HELP_name,
     /* disablement strs */ NULL, NULL }
};


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 *  Define the srptool Option Environment
 */
#define zPROGNAME       (srptool_opt_strs+1397)
#define zUsageTitle     (srptool_opt_strs+1405)
#define zRcName         NULL
#define apzHomeList     NULL
#define zBugsAddr       (srptool_opt_strs+1503)
#define zExplain        (srptool_opt_strs+1522)
#define zDetail         (srptool_opt_strs+1525)
#define zFullVersion    (srptool_opt_strs+1967)
/* extracted from optcode.tlib near line 350 */

#if defined(ENABLE_NLS)
# define OPTPROC_BASE OPTPROC_TRANSLATE | OPTPROC_NXLAT_OPT
  static tOptionXlateProc translate_option_strings;
#else
# define OPTPROC_BASE OPTPROC_NONE
# define translate_option_strings NULL
#endif /* ENABLE_NLS */


#define srptool_full_usage (NULL)

#define srptool_short_usage (srptool_opt_strs+1982)

#endif /* not defined __doxygen__ */

/*
 *  Create the static procedure(s) declared above.
 */
/**
 * The callout function that invokes the optionUsage function.
 *
 * @param pOptions the AutoOpts option description structure
 * @param pOptDesc the descriptor for the "help" (usage) option.
 * @noreturn
 */
static void
doUsageOpt(tOptions * pOptions, tOptDesc * pOptDesc)
{
    optionUsage(&srptoolOptions, SRPTOOL_EXIT_SUCCESS);
    /* NOTREACHED */
    (void)pOptDesc;
    (void)pOptions;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/**
 * Code to handle the debug option.
 *
 * @param pOptions the srptool options data structure
 * @param pOptDesc the option descriptor for this option.
 */
static void
doOptDebug(tOptions* pOptions, tOptDesc* pOptDesc)
{
    static struct {long rmin, rmax;} const rng[1] = {
        { 0 ,  9999 } };
    int  ix;

    if (pOptions <= OPTPROC_EMIT_LIMIT)
        goto emit_ranges;
    optionNumericVal(pOptions, pOptDesc);

    for (ix = 0; ix < 1; ix++) {
        if (pOptDesc->optArg.argInt < rng[ix].rmin)
            continue;  /* ranges need not be ordered. */
        if (pOptDesc->optArg.argInt == rng[ix].rmin)
            return;
        if (rng[ix].rmax == LONG_MIN)
            continue;
        if (pOptDesc->optArg.argInt <= rng[ix].rmax)
            return;
    }

    option_usage_fp = stderr;

emit_ranges:

    optionShowRange(pOptions, pOptDesc, (void *)rng, 1);
}
/* extracted from optmain.tlib near line 1113 */

/**
 * The directory containing the data associated with srptool.
 */
#ifndef  PKGDATADIR
# define PKGDATADIR ""
#endif

/**
 * Information about the person or institution that packaged srptool
 * for the current distribution.
 */
#ifndef  WITH_PACKAGER
# define srptool_packager_info NULL
#else
static char const srptool_packager_info[] =
    "Packaged by " WITH_PACKAGER

# ifdef WITH_PACKAGER_VERSION
        " ("WITH_PACKAGER_VERSION")"
# endif

# ifdef WITH_PACKAGER_BUG_REPORTS
    "\nReport srptool bugs to " WITH_PACKAGER_BUG_REPORTS
# endif
    "\n";
#endif
#ifndef __doxygen__

#endif /* __doxygen__ */
/**
 * The option definitions for srptool.  The one structure that
 * binds them all.
 */
tOptions srptoolOptions = {
    OPTIONS_STRUCT_VERSION,
    0, NULL,                    /* original argc + argv    */
    ( OPTPROC_BASE
    + OPTPROC_ERRSTOP
    + OPTPROC_SHORTOPT
    + OPTPROC_LONGOPT
    + OPTPROC_NO_REQ_OPT
    + OPTPROC_NO_ARGS
    + OPTPROC_GNUUSAGE
    + OPTPROC_MISUSE ),
    0, NULL,                    /* current option index, current option */
    NULL,         NULL,         zPROGNAME,
    zRcName,      zCopyright,   zLicenseDescrip,
    zFullVersion, apzHomeList,  zUsageTitle,
    zExplain,     zDetail,      optDesc,
    zBugsAddr,                  /* address to send bugs to */
    NULL, NULL,                 /* extensions/saved state  */
    optionUsage, /* usage procedure */
    translate_option_strings,   /* translation procedure */
    /*
     *  Indexes to special options
     */
    { INDEX_OPT_MORE_HELP, /* more-help option index */
      NO_EQUIVALENT, /* save option index */
      NO_EQUIVALENT, /* '-#' option index */
      NO_EQUIVALENT /* index of default opt */
    },
    11 /* full option count */, 8 /* user option count */,
    srptool_full_usage, srptool_short_usage,
    NULL, NULL,
    PKGDATADIR, srptool_packager_info
};

#if ENABLE_NLS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <autoopts/usage-txt.h>

static char* AO_gettext(char const* pz);
static void  coerce_it(void** s);

/**
 * AutoGen specific wrapper function for gettext.
 * It relies on the macro _() to convert from English to the target
 * language, then strdup-duplicates the result string.
 *
 * @param[in] pz the input text used as a lookup key.
 * @returns the translated text (if there is one),
 *   or the original text (if not).
 */
static char *
AO_gettext(char const* pz)
{
    char* pzRes;
    if (pz == NULL)
        return NULL;
    pzRes = _(pz);
    if (pzRes == pz)
        return pzRes;
    pzRes = strdup(pzRes);
    if (pzRes == NULL) {
        fputs(_("No memory for duping translated strings\n"), stderr);
        exit(SRPTOOL_EXIT_FAILURE);
    }
    return pzRes;
}

static void coerce_it(void** s) { *s = AO_gettext(*s);
}

/**
 * Translate all the translatable strings in the srptoolOptions
 * structure defined above.  This is done only once.
 */
static void
translate_option_strings(void)
{
    tOptions * const pOpt = &srptoolOptions;

    /*
     *  Guard against re-translation.  It won't work.  The strings will have
     *  been changed by the first pass through this code.  One shot only.
     */
    if (option_usage_text.field_ct != 0) {
        /*
         *  Do the translations.  The first pointer follows the field count
         *  field.  The field count field is the size of a pointer.
         */
        tOptDesc * pOD = pOpt->pOptDesc;
        char **    ppz = (char**)(void*)&(option_usage_text);
        int        ix  = option_usage_text.field_ct;

        do {
            ppz++;
            *ppz = AO_gettext(*ppz);
        } while (--ix > 0);

        coerce_it((void*)&(pOpt->pzCopyright));
        coerce_it((void*)&(pOpt->pzCopyNotice));
        coerce_it((void*)&(pOpt->pzFullVersion));
        coerce_it((void*)&(pOpt->pzUsageTitle));
        coerce_it((void*)&(pOpt->pzExplain));
        coerce_it((void*)&(pOpt->pzDetail));
        coerce_it((void*)&(pOpt->pzPackager));
        coerce_it((void*)&(pOpt->pzShortUsage));
        option_usage_text.field_ct = 0;

        for (ix = pOpt->optCt; ix > 0; ix--, pOD++)
            coerce_it((void*)&(pOD->pzText));
    }

    if ((pOpt->fOptSet & OPTPROC_NXLAT_OPT_CFG) == 0) {
        tOptDesc * pOD = pOpt->pOptDesc;
        int        ix;

        for (ix = pOpt->optCt; ix > 0; ix--, pOD++) {
            coerce_it((void*)&(pOD->pz_Name));
            coerce_it((void*)&(pOD->pz_DisableName));
            coerce_it((void*)&(pOD->pz_DisablePfx));
        }
        /* prevent re-translation */
        srptoolOptions.fOptSet |= OPTPROC_NXLAT_OPT_CFG | OPTPROC_NXLAT_OPT;
    }
}

#endif /* ENABLE_NLS */

#ifdef  __cplusplus
}
#endif
/* srptool-args.c ends here */
