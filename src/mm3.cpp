#include "hls_vector.h"
#include "hls_stream.h"
#include "ap_int.h"
#include "mm.h"

const int WIDTH = 64/sizeof(DTYPE);

extern "C" {

void mm(DTYPE *A,  DTYPE *B, DTYPE *AB,   int N )
{
#pragma HLS INTERFACE mode=m_axi bundle=m0 port=A 
#pragma HLS INTERFACE mode=m_axi bundle=m1 port=B 
#pragma HLS INTERFACE mode=m_axi bundle=m1 port=AB 

    for(int i=0;i<N;i++){
        for(int j=0;j<N;j++){
            DTYPE temp = 0;
            for(int k=0;k<N;k++){
                temp += A[i*N+k] * B[k*N+j];
            }
            AB[i*N+j] = temp;
        }
    }

}
}