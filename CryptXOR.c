#include "CryptXOR.h"

void CryptXOR(char *src, char *dst, unsigned char *key, unsigned int keysize)
{
	unsigned char tmpdata[CPT_MAX_READ_SIZE];
	int keypos = 0, n, i;
	long hnd_src = open(src, O_RDONLY), hnd_dst, tam_total = 0;

// Se ocorrer erro na abertura do arquivo de origem, retorna.
	if(hnd_src<0)
		return;

	if(!dst)
		{
		close(hnd_src);

 		hnd_src = open(src, O_RDWR);
 		hnd_dst = hnd_src;

// Se ocorrer erro na abertura do arquivo, retorna.
		if(hnd_src<0)
			return;
		}
	else
		{
 		hnd_dst = open(dst, O_CREAT | O_TRUNC | O_WRONLY, 0600);

// Se ocorrer erro na abertura do arquivo de destino, fecha o de origem e retorna.
		if(hnd_dst<0)
			{
			close(hnd_src);
			return;
			}
		}

	while((n = read(hnd_src, tmpdata, CPT_MAX_READ_SIZE))>0)
		{
		for(i=0; i<n; i++) // Loop de encriptação
			{
			tmpdata[i] ^= key[keypos++]; // Criptografa o dado
			if(keypos == keysize) // Verifica se alcançou o fim da chave
				keypos = 0; // Se alcançou, recomeça do zero
			}

// Se o arquivo de origem for o mesmo arquivo de destino, seleciona a posição.
// Não teria problema chamar o seek se os arquivos fossem difernetes porém
// iria tornar o processo mais lento.
		if(hnd_dst == hnd_src)
			{
			lseek(hnd_dst, SEEK_SET, tam_total);
			tam_total += n;
			}

		write(hnd_dst, tmpdata, n); // Grava os dados no arquivo de destino
		}

	close(hnd_src);
	if(hnd_src != hnd_dst)
		close(hnd_dst);
}
