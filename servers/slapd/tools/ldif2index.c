#include "portable.h"

#include <stdio.h>

#include <ac/stdlib.h>

#include <ac/ctype.h>
#include <ac/string.h>
#include <ac/socket.h>
#include <ac/unistd.h>

#include "../slap.h"
#include "../back-ldbm/back-ldbm.h"

#include "ldapconfig.h"
#include "ldif.h"

#define MAXARGS      		100

static void
usage( char *name )
{
	fprintf( stderr, "usage: %s -i inputfile [-d debuglevel] [-f configfile] [-n databasenumber] attr\n", name );
	exit( 1 );
}

int
main( int argc, char **argv )
{
	int		i, stop;
	char		*tailorfile, *inputfile;
	char		*linep, *buf, *attr;
	char		line[BUFSIZ];
	int		lineno, elineno;
	int      	lmax, lcur, indexmask, syntaxmask;
	int		dbnum;
	unsigned long	id;
	Backend		*be = NULL;
	struct ldbminfo *li;
	struct berval	bv;
	struct berval	*vals[2];

	ldbm_ignore_nextid_file = 1;

	inputfile = NULL;
	tailorfile = SLAPD_DEFAULT_CONFIGFILE;
	dbnum = -1;
	while ( (i = getopt( argc, argv, "d:f:i:n:" )) != EOF ) {
		switch ( i ) {
		case 'd':	/* turn on debugging */
			ldap_debug = atoi( optarg );
			break;

		case 'f':	/* specify a tailor file */
			tailorfile = strdup( optarg );
			break;

		case 'i':	/* input file */
			inputfile = strdup( optarg );
			break;

		case 'n':	/* which config file db to index */
			dbnum = atoi( optarg ) - 1;
			break;

		default:
			usage( argv[0] );
			break;
		}
	}
	attr = attr_normalize( argv[argc - 1] );
	if ( inputfile == NULL ) {
		usage( argv[0] );
	} else {
		if ( freopen( inputfile, "r", stdin ) == NULL ) {
			perror( inputfile );
			exit( 1 );
		}
	}

	slap_init(SLAP_TOOL_MODE, ch_strdup(argv[0]));
	read_config( tailorfile );

	if ( dbnum == -1 ) {
		for ( dbnum = 0; dbnum < nbackends; dbnum++ ) {
			if ( strcasecmp( backends[dbnum].be_type, "ldbm" )
			    == 0 ) {
				break;
			}
		}
		if ( dbnum == nbackends ) {
			fprintf( stderr, "No ldbm database found in config file\n" );
			exit( 1 );
		}
	} else if ( dbnum < 0 || dbnum > (nbackends-1) ) {
		fprintf( stderr, "Database number selected via -n is out of range\n" );
		fprintf( stderr, "Must be in the range 1 to %d (number of databases in the config file)\n", nbackends );
		exit( 1 );
	} else if ( strcasecmp( backends[dbnum].be_type, "ldbm" ) != 0 ) {
		fprintf( stderr, "Database number %d selected via -n is not an ldbm database\n", dbnum );
		exit( 1 );
	}

	slap_startup(dbnum);

	be = &backends[dbnum];

	/* disable write sync'ing */
	li = (struct ldbminfo *) be->be_private;
	li->li_dbcachewsync = 0;

	attr_masks( be->be_private, attr, &indexmask, &syntaxmask );
	if ( indexmask == 0 ) {
		exit( 0 );
	}

	id = 0;
	stop = 0;
	lineno = 0;
	buf = NULL;
	lcur = lmax = 0;
	vals[0] = &bv;
	vals[1] = NULL;
	while ( ! stop ) {
		char		*type, *val, *s;
		int		vlen;

		if ( fgets( line, sizeof(line), stdin ) != NULL ) {
			int     len;

			lineno++;
			len = strlen( line );
			while ( lcur + len + 1 > lmax ) {
				lmax += BUFSIZ;
				buf = (char *) ch_realloc( buf, lmax );
			}
			strcpy( buf + lcur, line );
			lcur += len;
		} else {
			stop = 1;
		}
		if ( line[0] == '\n' || stop && buf && *buf ) {
			if ( *buf != '\n' ) {
				if (isdigit((unsigned char) *buf)) {
					id = atol(buf);
				} else {
					id++;
				}
				s = buf;
				elineno = 0;
				while ( (linep = ldif_getline( &s )) != NULL ) {
					elineno++;
					if ( ldif_parse_line( linep, &type, &val,
					    &vlen ) != 0 ) {
						Debug( LDAP_DEBUG_PARSE,
			    "bad line %d in entry ending at line %d ignored\n",
						    elineno, elineno, 0 );
						continue;
					}

					if ( strcasecmp( type, attr ) == 0 ) {
						bv.bv_val = val;
						bv.bv_len = vlen;
						index_add_values( be, attr,
						    vals, id );
					}
				}
			}
			*buf = '\0';
			lcur = 0;
		}
	}

	slap_shutdown(dbnum);
	slap_destroy();

	return( 0 );
}
