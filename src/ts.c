#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <inttypes.h>
#include <regex.h>
#include <time.h>
#include <string.h>
#include <sys/time.h>

static const char *microsec_placeholder_str = ".XXXXXX";

int does_if_strftime_result_buffer_overflow( char *tmbuf, size_t tmbuf_size, char *new_fmt )
{
        time_t t;
        struct tm *tmp;

        t = time(NULL);
        tmp = localtime(&t);

	if ( strftime( tmbuf, tmbuf_size , new_fmt, tmp ) == 0 )
	{
		fprintf(stderr, "[ERROR] Format string: '%s' applied to strftime() exceeds buffer. Buffersize: %" PRIuPTR "\n", new_fmt, tmbuf_size );
		return 1;
	}

	return 0;
}

char *check_time_fmt_for_msec_identifier(char *result, size_t result_size, char *fmt )
{
	const char *regexp = "(%\\.[^%sS]*(s|S))";
	char *result_microseconds_placeholder_ptr = NULL;

	regex_t reg_exp;
	const int MAX_MATCHES = 3;
	regmatch_t matches[MAX_MATCHES];

	int rv;
	rv = regcomp(&reg_exp, regexp, REG_EXTENDED);
	if ( rv )
		printf( "could not parse regular expression: regcomp() error code is: %d\n", rv );



	if ( ( rv = regexec( &reg_exp, fmt, MAX_MATCHES, matches, 0 ) ) == 0 )
	{

		char *dest_ptr;

		size_t size_including_percent_sign = matches[0].rm_so + 1;  /* +1 because of '%' */
		strncpy( result, fmt, size_including_percent_sign );

		/* remaining seconds format data */
		dest_ptr = result + size_including_percent_sign;
		char *remaining_sec_fmt_pos_src = fmt + matches[0].rm_so + 2; /* +2 because of '%' and '.' */
		size_t remaining_sec_identifier_size = matches[0].rm_eo -  matches[0].rm_so - 2 ; /* -2 one for '%' and one for the '.' */
		strncpy( dest_ptr, remaining_sec_fmt_pos_src, remaining_sec_identifier_size );

		/* write placeholder space for 6 digit microseconds placement later */
		dest_ptr += remaining_sec_identifier_size;
		strncpy( dest_ptr , microsec_placeholder_str, strlen( microsec_placeholder_str ));
		result_microseconds_placeholder_ptr = dest_ptr + 1 ; /* + 1 because of '.' in the microsec_placeholder_str */

		/* remaining format data after placeholder */
		dest_ptr += strlen( microsec_placeholder_str );
		char *remaining_fmt_pos_src = fmt + matches[0].rm_eo;
		size_t remaining_fmt_size = strlen( fmt + matches[0].rm_eo - 1 ); /* -1 because of the '.' */
		strncpy( dest_ptr, remaining_fmt_pos_src, remaining_fmt_size );
	}

	return result_microseconds_placeholder_ptr;
}

int main( int argc, char **argv )
{
	char ch;
	const size_t BUFFER_SIZE = 4096;
	char buffer[ BUFFER_SIZE ];
	size_t cnt = 0;
	char restore;

	char *fmt = "%b %d %H:%M:%S"; /* default format */
	const size_t new_fmt_size = 256;
	char new_fmt[new_fmt_size];
	char *microsec_placeholder_ptr = NULL;
	char *microsec_placeholder_ptr_end = NULL;

	struct tm *nowtm;
	struct timeval tv;
	char tmbuf[512];
	int do_write_timestamp = 1;

	memset( buffer, '\0', sizeof( buffer ) );
	memset( new_fmt, '\0', sizeof( new_fmt ) );
	memset( tmbuf, '\0', sizeof( tmbuf ) );

	if ( argc > 1 )
	{
		fmt = argv[1];
		if ( strlen( fmt ) > new_fmt_size )
		{
			fprintf(stderr, "[ERROR] Provided format: '%s' to long. Max: %" PRIuPTR "\n", fmt, new_fmt_size );
			return EXIT_FAILURE;
		}

		if ( NULL == ( microsec_placeholder_ptr = check_time_fmt_for_msec_identifier( new_fmt, new_fmt_size, fmt ) ) )
		{
			/* use the fmt provided as no microseconds are requested */	
			strncpy( new_fmt, argv[1], strlen( argv[1] ) );
		}
		else
		{
			microsec_placeholder_ptr_end = microsec_placeholder_ptr
				+ strlen( microsec_placeholder_str ) - 1 ; /* skip '.' */
		}
			
	}
	else
	{
		/* use default format as non custom format was provided */
		strncpy( new_fmt, fmt, strlen( fmt ) );
	}

	if ( does_if_strftime_result_buffer_overflow( tmbuf, sizeof( tmbuf ), new_fmt ) )
		return EXIT_FAILURE;

	while( read(STDIN_FILENO, &ch, 1 ) > 0 )
	{
		if ( do_write_timestamp )
		{
			gettimeofday(&tv, NULL);
			nowtm = localtime(&tv.tv_sec);

			if ( microsec_placeholder_ptr )
			{
				restore = *microsec_placeholder_ptr_end;
				sprintf( microsec_placeholder_ptr  , "%06ld", tv.tv_usec );
				*microsec_placeholder_ptr_end = restore;
			}

			strftime( tmbuf, sizeof tmbuf, new_fmt, nowtm );
			dprintf( STDOUT_FILENO, "%s ", tmbuf );
		

			do_write_timestamp = 0;
		}

		if ( cnt == BUFFER_SIZE )
		{
			dprintf( STDOUT_FILENO, "%s", buffer );
			cnt = 0;
		}

		buffer[cnt] = ch;

		if ( ch == '\n' )
		{

			buffer[cnt + 1 ] = '\0';

			dprintf( STDOUT_FILENO, "%s", buffer );

			cnt = 0;
			do_write_timestamp = 1;

		}
		else
		{
			cnt++;
		}
	}

	return EXIT_SUCCESS;
}
