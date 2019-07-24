// Last Update:2019-07-24 17:49:06

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

#define NALU_TYPE_IDR 5
#define NALU_TYPE_SEI 6
#define NALU_TYPE_SPS 7
#define NALU_TYPE_PPS 8
#define NALU_TYPE_NON_IDR 1

int h264_annexb2avcc( char *in, char *out )
{
    FILE *fi = fopen( in, "r");
    FILE *fo = fopen( out, "w+");
    struct stat statbuf;
    char *buf_ptr = NULL, *buf_end = NULL, *start = NULL;
    unsigned char startcode[4] = { 0x00, 0x00, 0x00, 0x01 };
    int len = 0, i = 0, last_nalu_type = 0, nalu_type = 0;

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
            nalu_type = buf_ptr[4] & 0x1f;
            //LOGI("nalu_type : %d\n", nalu_type );
            if ( last_nalu_type == NALU_TYPE_SEI ) {
                if (nalu_type == NALU_TYPE_SPS ) {
                    LOGI("meet sps or non idr, the last is sei, skip\n");
                    last_nalu_type = nalu_type;
                    buf_ptr++;
                    continue;
                } else {
                    LOGI("warning: the nalu after sei is not sps, nalu_type : %d\n", nalu_type );
                }
            } 

            if ( last_nalu_type == NALU_TYPE_SPS ) {
                if ( nalu_type == NALU_TYPE_PPS) {
                    LOGI("meet pps, the last is sps, skip\n");
                    last_nalu_type = nalu_type;
                    buf_ptr++;
                    continue;
                } else {
                    LOGI("warning: the nalu after sps is not pps, nalu_type: %d\n", nalu_type );
                }
            } 

            if ( last_nalu_type == NALU_TYPE_PPS ) {
                if ( nalu_type == NALU_TYPE_IDR) {
                    LOGI("meet idr, the last is pps, skip\n");
                    last_nalu_type = nalu_type;
                    buf_ptr++;
                    continue;
                } else {
                    LOGI("warning: the nalu after pps is not idr, nalu_type : %d\n", nalu_type);
                }
            }

            last_nalu_type = nalu_type;
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
