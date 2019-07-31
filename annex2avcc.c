// Last Update:2019-07-31 12:39:35

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
#define NALU_TYPE_AUD 9

int dump_stream( int nalu_type )
{
    switch( nalu_type ) {
    case NALU_TYPE_SEI:
        printf("|  SEI " );
        break;
    case NALU_TYPE_SPS:
        printf("|  SPS " );
        break;
    case NALU_TYPE_PPS:
        printf("|  PPS ");
        break;
    case NALU_TYPE_IDR:
        printf("|  IDR ");
        break;
    case NALU_TYPE_NON_IDR:
        printf("|NONIDR");
        break;
    case NALU_TYPE_AUD:
        printf("|AUD");
        break;
    default:
        LOGE("error, unknow nalu type, nalu_type = %d\n", nalu_type );
        return -1;
    }

    return 0;
}

int h264_annexb2avcc( char *in, char *out )
{
    FILE *fi = fopen( in, "r");
    FILE *fo = fopen( out, "w+");
    struct stat statbuf;
    char *buf_ptr = NULL, *buf_end = NULL, *start = NULL;
    unsigned char startcode3[3] = { 0x00, 0x00, 0x01 };
    unsigned char startcode[4] = { 0x00, 0x00, 0x00, 0x01 };
    int len = 0, i = 0, last_nalu_type = 0, nalu_type = 0, startcode_len = 0;

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
            startcode_len = 4;
            i++;
        } else if ( memcmp(startcode3, buf_ptr, 3) == 0 ) {
            nalu_type = buf_ptr[3] & 0x1f;
            startcode_len = 3;
            i++;
        } else {
            buf_ptr++;
            continue;
        }


        if ( startcode_len ) {

            dump_stream( nalu_type );
            
#if 0
            if ( last_nalu_type == NALU_TYPE_SEI ) {
                //LOGI("meet sps or non idr, the last is sei, skip\n");
                last_nalu_type = nalu_type;
                if ( !start )
                    start = buf_ptr;
                buf_ptr += startcode_len;
                continue;
            } 
#endif

            if ( last_nalu_type == NALU_TYPE_SPS ) {
                if ( nalu_type == NALU_TYPE_PPS) {
                    //LOGI("meet pps, the last is sps, skip\n");
                    last_nalu_type = nalu_type;
                    buf_ptr += startcode_len;
                    continue;
                } else {
                    LOGI("warning: the nalu after sps is not pps, nalu_type: %d\n", nalu_type );
                }
            } 

            if ( last_nalu_type == NALU_TYPE_PPS ) {
                if ( nalu_type == NALU_TYPE_IDR) {
                    //LOGI("meet idr, the last is pps, skip\n");
                    last_nalu_type = nalu_type;
                    buf_ptr += startcode_len;
                    continue;
                } else {
                    LOGI("warning: the nalu after pps is not idr, nalu_type : %d\n", nalu_type);
                }
            }

            //LOGI("last_nalu_type = %d\n", last_nalu_type );
            if ( start && last_nalu_type != NALU_TYPE_AUD /*&& last_nalu_type != NALU_TYPE_SEI*/ ) {
                len = buf_ptr - start;
                fwrite( &len, 4, 1, fo );
                fwrite( start, len, 1, fo );
            }
            last_nalu_type = nalu_type;
            start = buf_ptr;
            buf_ptr += startcode_len;
            startcode_len = 0;
        }
    }
    fclose( fo );

    LOGI("totol %d nalu units\n", i );
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
