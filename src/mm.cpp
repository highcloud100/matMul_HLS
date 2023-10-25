#include "hls_vector.h"
#include "hls_stream.h"
#include "ap_int.h"
#include "mm.h"

const int DSIZE = 64 / sizeof(DTYPE);

extern "C"
{

	// DSize는 bandwidht size와 맞춤
	// bandwidth 늘려서 병목 완화
	void mm(DTYPE *A, hls::vector<DTYPE, DSIZE> *B, hls::vector<DTYPE, DSIZE> *AB, int N)
	{
#pragma HLS INTERFACE mode = m_axi bundle = m0 port = A
#pragma HLS INTERFACE mode = m_axi bundle = m1 port = B
#pragma HLS INTERFACE mode = m_axi bundle = m1 port = AB

		DTYPE AB_block[M][M]; // 256 x 256
#pragma HLS ARRAY_PARTITION dim = 2 type = complete variable = AB_block

		DTYPE Bj[M];
#pragma HLS ARRAY_PARTITION dim = 1 type = complete variable = Bj

	ib_loop:
		for (int ib = 0; ib < N / M; ib++)
		{
		jb_loop:
			for (int jb = 0; jb < N / M; jb++)
			{
			// init AB_block
			init_i_loop:
				for (int i = 0; i < M; i++)
				{
#pragma HLS pipeline II = 1
				init_j_loop:
					for (int j = 0; j < M; j++)
					{
#pragma HLS unroll
						AB_block[i][j] = 0;
					}
				}

			// calculate AB_block
			kb_loop:
				for (int kb = 0; kb < N / M; kb++)
				{
				k_loop:
					for (int k = 0; k < M; k++)
					{
						// read Bj
						// 					readB_j_loop: for(int j = 0; j < M; j++) {
						// #pragma HLS pipeline II = 1
						// 						DTYPE B_temp = B[(kb*M+k)*N+jb*M+j];
						// 						Bj[j] = B_temp;
						// 					}
						for (int jj = 0; jj < M / DSIZE; jj++)
						{ // m size의 배열이 필요하기에 m/DSIZE만큼 반복
#pragma HLS pipeline II = 1
							hls::vector<DTYPE, DSIZE> B_temp = B[((kb * M + k) * N + jb * M) / DSIZE + jj]; // 벡터를 외부 메모리에서 읽어옴
							for (int j = 0; j < DSIZE; j++)
							{
#pragma HLS unroll
								Bj[jj * DSIZE + j] = B_temp[j]; // Bj에 순차적으로 복사
							}
						}

					// read Ai and calculate AB_block
					i_loop:
						for (int i = 0; i < M; i++)
						{
#pragma HLS pipeline II = 1
							DTYPE Ai = A[((ib * M + i) * N + kb * M) + k];
						j_loop:
							for (int j = 0; j < M; j++)
							{
#pragma HLS unroll
								AB_block[i][j] += Ai * Bj[j];
							}
						}
					}
				}
				for (int i = 0; i < M; i++)
				{
					for (int jj = 0; jj < M / DSIZE; jj++)
					{
#pragma HLS pipeline II = 1
						hls::vector<DTYPE, DSIZE> AB_temp;
						for (int j = 0; j < DSIZE; j++)
						{ // block에서 배열 하나를 읽어 AB_temp에 저장
#pragma HLS unroll
							AB_temp[j] = AB_block[i][jj * DSIZE + j];
						}
						AB[((ib * M + i) * N + jb * M) / DSIZE + jj] = AB_temp; // 외부로 AB_temp를 쓰기
					}
				}

				// 			writeAB_i_loop: for(int i = 0; i < M; i++) {
				// 				writeAB_j_loop: for(int j = 0; j < M; j++) {
				// #pragma HLS pipeline II = 1
				// 					AB[(ib*M+i)*N+jb*M+j] = AB_block[i][j];
				// 				}
				// 			}
			}
		}
	}
}
