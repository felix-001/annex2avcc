// Last Update:2019-07-24 15:43:46

#include <stdio.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>

#define LOGI(args...) printf( "%s:%d(%s) -- ", __FILE__, __LINE__, __FUNCTION__);printf(args)
#define LOGE(args...) LOGI(args)

#define MALLOC_CHAR( ptr, size ) do { \
    ptr = (char *)malloc(size); \
    if ( !ptr ) { \
        LOGE("malloc error\n"); \
        goto err; \
    } \
    memset( ptr, 0, size ); \
}while(0)

int h264_annexb2avcc( char *in, char *out )
{
    FILE *fi = fopen( in, "r");
    FILE *fo = fopen( out, "w+");
    struct stat statbuf;
    char *buf_ptr = NULL, *buf_end = NULL, *start = NULL;
    unsigned char startcode[4] = { 0x00, 0x00, 0x00, 0x01 };
    int len = 0, i = 0;

    if ( !fi || !fo ) {
        LOGE("open file %s or %s error\n", in, out );
        goto err;
    }

    if ( stat( in, &statbuf ) == -1 ) {
        LOGE("get file %s stat error\n", in );
        goto err;
    }
    LOGI("file %s size is : %d\n", in, (int)statbuf.st_size );
    MALLOC_CHAR( buf_ptr, (int)statbuf.st_size );
    buf_end = buf_ptr + (int)statbuf.st_size;
    fread( buf_ptr, (int)statbuf.st_size, 1, fi );
    fclose( fi );
    fi = NULL;

    while( buf_ptr <= buf_end ) {
        if ( memcmp(startcode, buf_ptr, 4) == 0 ) {
            i++;
            if ( start ) {
                len = buf_ptr - start;
                fwrite( &len, 4, 1, fo );
                fwrite( start, len, 1, fo );
            }
            start = buf_ptr;
        }
        buf_ptr++;
    }
    fclose( fo );

    LOGI("totol %d frames\n", i );
    return 0;
err:
    if ( fi )
        fclose( fi );
    if ( fo )
        fclose(fo);
    return -1;
}

int main( int argc, char *argv[] )
{
    if ( argc < 2 ) {
        printf("usage : annexb2avcc [infile] [outfile]\n");
        return 0;
    }

    h264_annexb2avcc( argv[1], argv[2] );

    return 0;
}
