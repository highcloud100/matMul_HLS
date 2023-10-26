#include "hls_vector.h"
#include "hls_stream.h"
#include "ap_int.h"
#include "mm.h"
#include "hls_print.h"

const int DSIZE = 64 / sizeof(DTYPE);

extern "C"
{

	void ChangeA_Rate(hls::stream<hls::vector<DTYPE, DSIZE>> & AStreamWide, 
		hls::stream<DTYPE> & AStream, int N){
			for(int ib=0;ib<N/M;ib++){
				for(int jb=0;jb< N/M; jb++){
					for(int kb = 0;kb < N/M;kb++){
						for(int k=0;k<M;k++){
							
							for(int ii=0;ii <M/DSIZE;ii++){ // M size의 배열이 필요하기에 m/DSIZE만큼 반복
							// hls::print("changeA_rate : AStreamWide reading\n");
								hls::vector<DTYPE, DSIZE> A_temp = AStreamWide.read();
								for(int i=0;i<DSIZE;i++){
									///	hls::print("changeA_rate : AStream writing\n");
									AStream.write(A_temp[i]);
									//hls::print("AStream : %d\n", A_temp[i]);
								}
								//hls::print("--------------------\n");
							}

					}
					}
				}
			}
			 hls::print("changeA_rate end\n");
		}

	void ReadAt( hls::vector<DTYPE, DSIZE> *At, hls::stream<hls::vector<DTYPE, DSIZE>> & AStreamWide, int N){
		for(int kb=0;kb<N/M;kb++){ // 오른쪽 블럭 column 이동
			for(int jb=0;jb<N/M;jb++){ // dup
				for(int ib=0;ib< N/M; ib++){ // 블럭 아래로 하나씩 
					for(int i=0;i<M;i++){ // 블럭 내에서 아래로 이동
						for(int k=0;k<M/DSIZE;k++){ // 블럭 내에서 한줄 읽기
							//hls::print("ReadAt : AStreamWide writing\n");
							AStreamWide.write(At[((ib*M+i)*N + kb*M)/DSIZE + k]);
						}
					}
				}
			}
			
		}
		 hls::print("ReadAt end\n");
	}

	void ReadB(hls::vector<DTYPE, DSIZE> *B, hls::stream<hls::vector<DTYPE, DSIZE>> & BStream, int N){
		for(int ib=0;ib<N/M;ib++){ // 반복
			for(int jb=0;jb<N/M;jb++){ // 오른쪽으로 이동
				for(int kb=0;kb<N/M;kb++){ // 아래로 이동 (블록)
					for(int k=0;k<M;k++){ // 블록 내에서 아래로 이동 (행)
						for(int jj=0;jj<M/DSIZE;jj++){ // 블록 내에서 한줄 읽기
						 //	hls::print("ReadB : BStream writing\n");
							BStream.write(B[((kb*M+k)*N+jb*M)/DSIZE+jj]);
						}
					}
				}
			}
			
		}
		 hls::print("ReadB\n");
	}

	void Comp(hls::stream<DTYPE> & AStream, hls::stream<hls::vector<DTYPE, DSIZE>> & BStream, hls::stream<hls::vector<DTYPE, DSIZE>> & ABStream, int N){
		for(int ib=0;ib < N/M;ib++){
			for(int jb=0;jb <N/M;jb++){
				for(int kb=0;kb< N/M;kb++){
					for(int k=0;k<M;k++){
						// read Bj
						auto temp = BStream.read();
						for(int i=0;i<M;i++){
							DTYPE Ai = AStream.read();
							ABStream.write(temp*Ai);
						}
					}
				}
			}
		}
		 hls::print("Comp end\n");
	}

	void WriteAB(hls::stream<hls::vector<DTYPE, DSIZE>> & ABStream, hls::vector<DTYPE, DSIZE> *AB, int N){
		for(int ib=0;ib<N/M;ib++){
			for(int jb=0;jb<N/M;jb++){
				//for(int kb=0;kb<N/M;kb++){
					for(int k=0;k<M;k++){
						for(int i=0;i<M;i++){
							
							
							//hls::print("writeAB : ABStream reading\n");
							AB[((ib*M+i)*N+jb*M)/DSIZE] += ABStream.read();
							
							
						}
					}
				//}
			}
		}
		 hls::print("writeAB end\n");
	}

	void mm( hls::vector<DTYPE, DSIZE> *At, hls::vector<DTYPE, DSIZE> *B, hls::vector<DTYPE, DSIZE> *AB, int N)
	{
#pragma HLS INTERFACE mode = m_axi bundle = m0 port = At
#pragma HLS INTERFACE mode = m_axi bundle = m1 port = B
#pragma HLS INTERFACE mode = m_axi bundle = m1 port = AB

#pragma HLS DATAFLOW

	hls::stream<hls::vector<DTYPE, DSIZE> > AStreamWide("AStreamWide");
	hls::stream<DTYPE> AStream("AStream");
	hls::stream<hls::vector<DTYPE, DSIZE> > BStream("BStream");
	hls::stream<hls::vector<DTYPE, DSIZE> > ABStream("ABStream");

	ReadAt(At, AStreamWide, N);
	ChangeA_Rate(AStreamWide, AStream, N);
	ReadB(B, BStream, N);
	Comp(AStream, BStream, ABStream, N);
	WriteAB(ABStream, AB, N);
	}


}
