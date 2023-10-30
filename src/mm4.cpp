#include "hls_vector.h"
#include "hls_stream.h"
#include "ap_int.h"
#include "mm.h"
//#include "hls_print.h"

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
								hls::vector<DTYPE, DSIZE> A_temp = AStreamWide.read();
								for(int i=0;i<DSIZE;i++){
#pragma HLS PIPELINE
									AStream.write(A_temp[i]);
								}
							}

					}
					}
				}
			}
			// hls::print("changeA_rate end\n");
		}

	void ReadAt( hls::vector<DTYPE, DSIZE> *At, hls::stream<hls::vector<DTYPE, DSIZE>> & AStreamWide, int N){
		for(int kb=0;kb<N/M;kb++){ // 오른쪽 블럭 column 이동
			for(int jb=0;jb<N/M;jb++){ // dup
				for(int ib=0;ib< N/M; ib++){ // 블럭 아래로 하나씩 
					for(int i=0;i<M;i++){ // 블럭 내에서 아래로 이동
						for(int k=0;k<M/DSIZE;k++){ // 블럭 내에서 한줄 읽기
#pragma HLS PIPELINE
							AStreamWide.write(At[((ib*M+i)*N + kb*M)/DSIZE + k]);
							//hls::print("AstreamWide read\n");
						}
					}
				}
			}
			
		}
		// hls::print("ReadAt end\n");
	}

	void ReadB(hls::vector<DTYPE, DSIZE> *B, hls::stream<hls::vector<DTYPE, DSIZE>> & BStream, int N){
		for(int ib=0;ib<N/M;ib++){ // 반복
			for(int jb=0;jb<N/M;jb++){ // 오른쪽으로 이동
				for(int kb=0;kb<N/M;kb++){ // 아래로 이동 (블록)
					for(int k=0;k<M;k++){ // 블록 내에서 아래로 이동 (행)
						for(int jj=0;jj<M/DSIZE;jj++){ // 블록 내에서 한줄 읽기
#pragma HLS PIPELINE
							BStream.write(B[((kb*M+k)*N+jb*M)/DSIZE+jj]);
							//hls::print("Bstream read\n");
						}
					}
				}
			}
			
		}
		// hls::print("ReadB end\n");
	}

	void Comp(hls::stream<DTYPE> & AStream, hls::stream<hls::vector<DTYPE, DSIZE>> & BStream, hls::stream<hls::vector<DTYPE, DSIZE>> & ABStream, int N){
		for(int ib=0;ib < N/M;ib++){ // 블럭 오른쪽으로 이동
			for(int jb=0;jb <N/M;jb++){ // 블럭 아래로 이동
				DTYPE AB_block[M][M];
				for(int i=0;i<M;i++){
					for(int j=0;j<M;j++){
						AB_block[i][j] = 0;
					}
				}
				

 				for(int kb=0;kb< N/M;kb++){ // 중복
					for(int k=0;k<M;k++){ // 블럭 내에서 아래로 이동
						DTYPE Bj[M];

						for(int jj=0;jj<M/DSIZE;jj++){ // B 블록의 한줄 읽기 
							auto temp = BStream.read(); 
#pragma HLS PIPELINE
							for(int j=0;j<DSIZE;j++){
								Bj[j+jj*DSIZE] = temp[j];
							}
							//hls::print("Bj read\n");
						}


						for(int i=0;i<M;i++){ // A에서 M번 읽음 (한 줄 읽음)
							DTYPE Ai = AStream.read(); 
#pragma HLS PIPELINE
							for(int jj=0;jj<M;jj++){
								//ABStream.write(Ai*Bj[jj]);
								AB_block[i][jj] += Ai*Bj[jj];
							}
						}
					}
				}
				for(int k=0;k<M;k++){
					for(int i=0;i<M/DSIZE;i++){
#pragma HLS PIPELINE
						hls::vector<DTYPE, DSIZE> temp;
						for(int j=0;j<DSIZE;j++){
							temp[j] = AB_block[k][j+i*DSIZE];
						}
						ABStream.write(temp);
					//hls::print("ABstream write\n");
					}
				}
				
			}
		}
		 //hls::print("Comp end\n");
	}

	void WriteAB(hls::stream<hls::vector<DTYPE, DSIZE>> & ABStream, hls::vector<DTYPE, DSIZE> *AB, int N){
		for(int ib=0;ib<N/M;ib++){ // block 세로 이동
			for(int jb=0;jb<N/M;jb++){ // block 가로 이동
				//for(int kb=0;kb<N/M;kb++){ // cumulate
					//for(int k=0;k<M;k++){ // ?
						for(int i=0;i<M;i++){
							for(int jj=0;jj<M/DSIZE;jj++){ // 여기서는 write만
#pragma HLS PIPELINE
								AB[((ib*M+i)*N+jb*M)/DSIZE+jj] = ABStream.read();
								//hls::print("abstream reading\n");
							}
						}
					//}
				//}
			}
		}
		// hls::print("writeAB end\n");
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

	//hls::print("mm start\n");
	ReadAt(At, AStreamWide, N);
	ChangeA_Rate(AStreamWide, AStream, N);
	ReadB(B, BStream, N);
	Comp(AStream, BStream, ABStream, N);
	WriteAB(ABStream, AB, N);
	}


}
