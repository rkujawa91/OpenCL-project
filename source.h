#ifndef SOURCE_H_
#define SOURCE_H_
#include <stdio.h>

#define uchar unsigned char // 8-bit byte
#define uint unsigned long // 32-bit word

	void AddRoundKey(uchar state[][4], uint w[]);
	void aes_decrypt(uchar in[], uchar out[], uint key[], int keysize);
	void aes_encrypt(uchar in[], uchar out[], uint key[], int keysize);
	void InvMixColumns(uchar state[][4]);
	void InvShiftRows(uchar state[][4]);
	void InvSubBytes(uchar state[][4]);
	void KeyExpansion(uchar key[], uint w[], int keysize);
	void MixColumns(uchar state[][4]);
	void ShiftRows(uchar state[][4]);
	void SubBytes(uchar state[][4]);
	uint SubWord(uint word);

	unsigned char* Read_Source_File(const char *);

	void use_cpu_encryption(unsigned char*, unsigned char*, unsigned char*,
		unsigned char*, unsigned char*, unsigned char*, unsigned long* );

	int Use_OpenCL_encryption(unsigned char*, unsigned char*, unsigned char*,
		unsigned long*, unsigned char*, unsigned char*, int, int );
	
#endif
