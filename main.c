#include <stdio.h>
#include<time.h> // for clock


//#define SIZE 600000
#define BLOCK 16
int main()
{
   // This is one set of test vectors. Plaintext1 should encrypt to ciphertext1 using
   // key1 and the ciphertext1 should decrypt to the plaintext1 using key1.
   /*0x00,0x11,0x22,0x33,0x44,0x55,0x66,0x77,
                       0x88,0x99,0xaa,0xbb,0xcc,0xdd,0xee,0xff,0xff,0xee,0xdd,0xcc,0xbb,0xaa,0x99,0x88,
                       0x77,0x66,0x55,0x44,0x33,0x22,0x11,0x00*/
    FILE *file;
    const int SIZE = 600000;
    char buf[SIZE];

    size_t nread;
    int fileSize =0;
    file = fopen("test.txt", "r");
    clock_t start, end;
	double cpu_time_used;
	start = clock();
	printf(" start: %d\n", start);

    if (file) {
        while ((nread = fread(buf, 1, 1, file)) > 0)
        {
            fileSize++;
        }
        if (ferror(file)) {
        printf(" file\n");

            return -1;
        }
        fclose(file);
        file = fopen("test.txt", "r");
        while ((nread = fread(buf, 1, sizeof buf, file)) > 0)
        {
            //fileSize++;
        }
        if (ferror(file)) {
            return -1;
        }
        fclose(file);
    }
    int i =0,j=0;
    unsigned char data[SIZE];
    unsigned char encryptdata [SIZE];

    /**/
   unsigned char plaintext[BLOCK];
   unsigned char ciphertext[BLOCK];
   unsigned char lastCiphertext[BLOCK];
   unsigned char key[32]={0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,
                 0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0x10,0x11,
                 0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,
                 0x1b,0x1c,0x1d,0x1e,0x1f};


   unsigned int key_schedule[60],idx;


   // First set of test vectors.
   KeyExpansion(key,key_schedule,128);
   //* Encrypt Data
   for (i=0; i<fileSize; i++){
        plaintext[i%16] = buf[i];
        if(i%16==15){
            aes_encrypt(plaintext,ciphertext,key_schedule,128);
            for (j = i-15; j < i+1; j++){
                encryptdata[j] = ciphertext[j%16];
            }
            memset(plaintext,0,BLOCK);
            memset(ciphertext,0,BLOCK);
        }
        else if (i == fileSize-1){
            aes_encrypt(plaintext,lastCiphertext,key_schedule,128);
            for (j = i-(i%16); j < i+1; j++){
                encryptdata[j] = lastCiphertext[j%16];
            }
        }

   }   //*/

   //* Decrypt Data
   for (i=0; i<fileSize; i++){
        ciphertext[i%16] = encryptdata[i];
        if(i%16==15){
            aes_decrypt(ciphertext,plaintext,key_schedule,128);
            for (j = i-15; j < i+1; j++){
                data[j] = plaintext[j%16];
            }
            memset(plaintext,0,BLOCK);
            memset(ciphertext,0,BLOCK);
        }
        else if (i == fileSize-1){
            aes_decrypt(lastCiphertext,plaintext,key_schedule,128);
            for (j = i-(i%16); j < i+1; j++){
                data[j] = plaintext[j%16];
                }
        }

   }//*/

/*Print Encrypted data
   printf("Encrypted Data\n");
    for (idx=0; idx < fileSize; idx++)
      printf("%c",encryptdata[idx]);
//**/
printf("\nData Decrypted\n");
   for (idx=0; idx < fileSize; idx++)
      printf("%c",data[idx]); //*/

    printf("\nLast ciphertext\n");
   for (idx=0; idx < 16; idx++)
      printf("%02x",ciphertext[idx]);

   printf("\nLast plaintext\n");
   for (idx=0; idx < 16; idx++){
    printf("%c",plaintext[idx]);
   }


    end = clock();
	printf("\nEnd: %d\n", end);

	cpu_time_used = ((double)(end - start)) / CLOCKS_PER_SEC;
	printf ("Seconds: %f\n", cpu_time_used);

   return 0;
}

