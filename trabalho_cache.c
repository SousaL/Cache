//---------------------------------------------------------------
//Arquivo: trabalho_cache.c
//Autores: Leonardo Sousa
//---------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#define MP pow(2,32)
enum politicaDeEscrita{ 
	writeThrough,
	writeBack
};
enum politicaDeSubstituicao{
	LFU, LRU, ALEATORIO
};
enum r_w{
	READ = 'R', WRITE = 'W'
};

struct estruturaDaCache{
	unsigned int numeroBlocosNaMP;
	int tamanhoDoBloco;
	int tamanhoDoConjunto;
	int quantidadeDeConjuntos;
	int numeroDeLinhasNaCache;
	int tamanhoDaCache;
	int tempoCacheHit;
	int tempoMP;
	enum politicaDeEscrita modoEscrita;
	enum politicaDeSubstituicao modoSubstituicao;
};

struct endereco{
	int rotulo;
	int conjunto;
	int palavra;
};
struct conjunto{
	int *bloco;
	int *bitContador;
	int *dirtybit;
};

struct cache{
	struct conjunto *conjunto;
	struct estruturaDaCache *config;
	float escritaCache;
	float leituraCache;
	float leituras;
	float escritas;
	float tempoMedio;
	float leituraNaMP;//UTILIZADO QUANDO DIRTY BIT E WRITE BACK
	float escritaNaMP;
	float atualizaBlocoDirty;
	};
struct fita{
	int tamanho;
	unsigned int *endereco;
	char *tipoAcesso;
};
typedef struct fita FITA;
typedef struct conjunto CONJUNTO;
typedef struct endereco ENDERECO;
typedef struct estruturaDaCache CACHE_CONFIG;
typedef struct cache CACHE;
ENDERECO endereco;



CACHE_CONFIG *configurarCache(int TBloco, int TConjunto, int LCache, enum politicaDeEscrita MEscrita,
										enum politicaDeSubstituicao MSubtituicao, int TCache, int TMP){
						CACHE_CONFIG *tmp = (CACHE_CONFIG*)malloc(sizeof(CACHE_CONFIG));
						tmp->tamanhoDoBloco = TBloco;
						tmp->numeroBlocosNaMP = (unsigned int)(MP/TBloco);
						tmp->tamanhoDoConjunto = TConjunto;
						tmp->quantidadeDeConjuntos = LCache/TConjunto;
						tmp->numeroDeLinhasNaCache = LCache;
						tmp->tamanhoDaCache = LCache * TBloco;
						tmp->modoEscrita = MEscrita;
						tmp->modoSubstituicao = MSubtituicao;
						tmp->tempoCacheHit = TCache;
						tmp->tempoMP = TMP;
						endereco.palavra = (int)(log(TBloco)/log(2));
						endereco.conjunto = (int)(log(LCache/TConjunto)/log(2));
						endereco.rotulo = (int)((unsigned long)(log(MP) / log(2))) - endereco.palavra - endereco.conjunto;
						return tmp;
						}
				
CACHE *setarCache(){
	CACHE *cache = (CACHE*)malloc(sizeof(CACHE));
	int TBloco;
	int TConjunto;
	int LCache;
	enum politicaDeEscrita MEscrita;
	enum politicaDeSubstituicao MSubtituicao;
	int TCache;
	int TMP;
	printf("%-35s: ","Digite o tamanho da linha(Bloco)");
	scanf("%d",&TBloco);
	printf("%-35s: ","Digite o tamanho do conjunto");
	scanf("%d",&TConjunto);
	printf("%-35s: ","Digite a quantidade de linhas");
	scanf("%d",&LCache);
	printf("Digite a politica de escrita\n%-35s: ","0 - Write Through | 1 - Write Back");
	scanf("%d",&MEscrita);
	printf("Digite a politica de substituicao\n%-35s: ","0 - LFU | 1 - LRU | 2 - aleatorio");
	scanf("%d",&MSubtituicao);
	printf("%-35s: ","Digite o tempo da cache");
	scanf("%d",&TCache);
	printf("%-35s: ","Digite o tempo da memoria principal");
	scanf("%d",&TMP);
	
	
	cache->config = configurarCache(TBloco,TConjunto,LCache,MEscrita, MSubtituicao,TCache,TMP);
	cache->conjunto = (CONJUNTO*)malloc(sizeof(CONJUNTO) * cache->config->quantidadeDeConjuntos);
	cache->escritaCache = cache->leituraCache = cache->escritas = cache->leituras = cache->leituraNaMP = cache->escritaNaMP = 0.00;
	cache->atualizaBlocoDirty = 0.0;

	int i = 0;
	for(i = 0; i < cache->config->quantidadeDeConjuntos; i++){
		cache->conjunto[i].bloco = (int*)malloc(sizeof(int) * cache->config->tamanhoDoConjunto);
		memset(cache->conjunto[i].bloco, -1, sizeof(int) * cache->config->tamanhoDoConjunto);
		if(cache->config->modoSubstituicao == LFU){
			cache->conjunto[i].bitContador = (int*)malloc(sizeof(int) * cache->config->tamanhoDoConjunto);
			memset(cache->conjunto[i].bitContador, -1, sizeof(int) * cache->config->tamanhoDoConjunto);
		}
		if(cache->config->modoEscrita == writeBack){
			cache->conjunto[i].dirtybit = (int*)malloc(sizeof(int) * cache->config->tamanhoDoConjunto);
			memset(cache->conjunto[i].dirtybit, -1, sizeof(int) * cache->config->tamanhoDoConjunto);
		}
	}
	return cache;	
}

void lerCache(CACHE *cache, unsigned int endMemoria, enum r_w tipoAcesso){
	//-------------------------------------------------------------------------
	int i = 0;
	int tmp_rotulo = endereco.rotulo;
	int tmp_conjunto = endereco.conjunto;
	int valor_rotulo = 0, valor_conjunto = 0;
	if(tipoAcesso == READ)
			cache->leituras ++;
	else 
			cache->escritas ++;		
	while(tmp_rotulo > 0){
		valor_rotulo <<= 1;
		tmp_rotulo--;
		if(endMemoria & 0x80000000)
			valor_rotulo|= 0x01;
		endMemoria <<= 1;
	}
	while(tmp_conjunto > 0){
		valor_conjunto <<= 1;
		tmp_conjunto--;
		if(endMemoria & 0x80000000)
			valor_conjunto |= 0x01;
		endMemoria <<= 1;
	}
	
	int achou = 0, aux_rotulo = 0,aux_dirtybit, min = 0, pos = 0;
	
	for(i = 0; i < cache->config->tamanhoDoConjunto; i++){
		if(cache->conjunto[valor_conjunto].bloco[i] == valor_rotulo){
			achou = 1;
			pos = i;
			break;		
		}
	}
	
	if(achou){
		switch(cache->config->modoSubstituicao){
			case LRU:
				aux_rotulo = cache->conjunto[valor_conjunto].bloco[pos];
				for(i = pos;i > 0; i--)
					cache->conjunto[valor_conjunto].bloco[i] = cache->conjunto[valor_conjunto].bloco[i - 1];
					
				cache->conjunto[valor_conjunto].bloco[0] = aux_rotulo;
				if(cache->config->modoEscrita == writeBack)
					cache->conjunto[valor_conjunto].dirtybit[0] = 1;
				break;	
			case LFU:
				cache->conjunto[valor_conjunto].bitContador[i]++;	
				if(cache->config->modoEscrita == writeBack)
					cache->conjunto[valor_conjunto].dirtybit[0] = 1;
				break;
		}
		if(tipoAcesso == READ)
			cache->leituraCache++;
		else if(tipoAcesso == WRITE)
			cache->escritaCache++;
		if(tipoAcesso == WRITE && cache->config->modoEscrita == writeThrough)
			cache->escritaNaMP++;
	}
	else{
		switch(cache->config->modoSubstituicao){
			case LRU:
				if(cache->config->modoEscrita == writeBack && cache->conjunto[valor_conjunto].dirtybit[cache->config->tamanhoDoConjunto] == 1){
					cache->atualizaBlocoDirty++;
					cache->escritaNaMP++;
				}
				for(i = cache->config->tamanhoDoConjunto - 1;i > 0; i--){
						cache->conjunto[valor_conjunto].bloco[i] = cache->conjunto[valor_conjunto].bloco[i - 1];
				}
				cache->conjunto[valor_conjunto].bloco[0] = valor_rotulo;
				if(cache->config->modoEscrita == writeBack)
					cache->conjunto[valor_conjunto].dirtybit[0] = 0;
				break;
			case LFU:
				for(i = 1; i < cache->config->tamanhoDoConjunto; i++){
					if(cache->conjunto[valor_conjunto].bitContador[i] < cache->conjunto[valor_conjunto].bitContador[min])
						min = i;
				}
				if(cache->config->modoEscrita == writeBack && cache->conjunto[valor_conjunto].dirtybit[min] == 1){
					cache->atualizaBlocoDirty++;
					cache->escritaNaMP++;
				}
					
				cache->conjunto[valor_conjunto].bloco[min] = valor_rotulo;
				if(cache->config->modoEscrita == writeBack)
					cache->conjunto[valor_conjunto].dirtybit[min] = 0;
				cache->conjunto[valor_conjunto].bitContador[min] = 1;
				break;
			case ALEATORIO:
				min = (rand() % (cache->config->tamanhoDoConjunto - 1));
				if(cache->config->modoEscrita == writeBack && cache->conjunto[valor_conjunto].dirtybit[min] == 1){
					cache->atualizaBlocoDirty++;
					cache->escritaNaMP++;
				}
				cache->conjunto[valor_conjunto].bloco[min] = valor_rotulo;
				if(cache->config->modoEscrita == writeBack)
					cache->conjunto[valor_conjunto].dirtybit[min] = 0;
				break;
			}
			if(tipoAcesso == READ)
				cache->leituraNaMP++;
			else
				cache->escritaNaMP++;		
	}
	return;
}
void tempo(CACHE *cache){
	float tempo_leitura = (cache->leituraCache/cache->leituras) * cache->config->tempoCacheHit;
	tempo_leitura+= ((cache->leituras - cache->leituraCache)/cache->leituras) * (cache->config->tempoCacheHit + cache->config->tempoMP);
	float tempo_escrita = 0.0;
	if(cache->config->modoEscrita == writeThrough)
		tempo_escrita = cache->config->tempoCacheHit + cache->config->tempoMP;
	else{
		tempo_escrita = ((cache->escritaCache/cache->escritas)*cache->config->tempoCacheHit);
		tempo_escrita += ((cache->escritas - cache->escritaCache)/cache->escritas) * ((cache->config->tempoCacheHit + cache->config->tempoMP));
		tempo_escrita += (cache->atualizaBlocoDirty * (cache->config->tempoCacheHit + cache->config->tempoMP));
		tempo_escrita /= 1 + cache->atualizaBlocoDirty;
	}
		
		
	cache->tempoMedio = ((cache->leituras/(cache->leituras+cache->escritas))*tempo_leitura) + ((cache->escritas/(cache->leituras+cache->escritas))*tempo_escrita);
}
FITA *carregar(){
	char *nome = (char*)malloc(sizeof(char) * 100);
	arquivo: printf("%-35s: ","Digite o nome do arquivo");
	fflush(stdin);
	fgets(nome,100,stdin);
	nome[strlen(nome) - 1] = '\0';
	FILE *f = fopen(nome, "r");
	if(!f){
		goto arquivo;	
		printf("Arquivo nao existe!\n");
	}
		
	FITA *tmp = (FITA*)malloc(sizeof(FITA));
	int x = 0;
	while(!feof(f))
		if(fgetc(f) == '\n')
			x++;
	tmp->tamanho = 0;
	tmp->tamanho = x;
	tmp->endereco = (unsigned int*)malloc(sizeof(unsigned int) * tmp->tamanho);
	tmp->tipoAcesso = (char*)malloc(sizeof(char) * tmp->tamanho);
	fseek(f,0,SEEK_SET);
	int i = 0;
	while(!feof(f)){
		fscanf(f,"%x %c",&(tmp->endereco[i]),&(tmp->tipoAcesso[i]));
		i++;
	}
	return tmp;
}
void relatorioCache(CACHE *cache){
	char *nome = (char*)malloc(sizeof(char) * 100);
	arquivo: printf("%-35s: ","Digite o nome do arquivo de saida");
	fflush(stdin);
	fgets(nome,100,stdin);
	nome[strlen(nome) - 1] = '\0';
	FILE *f = fopen(nome, "w");
	fprintf(f,"PARAMETROS DE ENTRADA\n--------------------------------------------------------\n");
	fprintf(f,"%-30s: %d bytes\n","Tamanho do bloco",cache->config->tamanhoDoBloco);
	fprintf(f,"%-30s: %d blocos\n","Quantidade de blocos na MP",cache->config->numeroBlocosNaMP);
	fprintf(f,"%-30s: %d blocos\n","Tamanho do conjunto",cache->config->tamanhoDoConjunto);
	fprintf(f,"%-30s: %d conjuntos\n","Quantidade de conjuntos",cache->config->quantidadeDeConjuntos);
	fprintf(f,"%-30s: %d linhas\n","Numero de linhas na cache",cache->config->numeroDeLinhasNaCache);
	fprintf(f,"%-30s: %d bytes\n","Tamanho da cache",cache->config->tamanhoDaCache);
	fprintf(f,"%-30s: %s\n","Modo escrita",cache->config->modoEscrita == writeBack ? "Write Back" : "Write Through");
	fprintf(f,"%-30s: %s\n","Modo de subsituicao",cache->config->modoSubstituicao == LRU ? "LRU" : (cache->config->modoSubstituicao == LRU ? "LFU" : "Aleatorio"));
	fprintf(f,"%-30s: %d ns\n","Tempo de Cache",cache->config->tempoCacheHit);
	fprintf(f,"%-30s: %d ns\n","Tempo de MP",cache->config->tempoMP);
	fprintf(f,"%-30s: %d bits\n","Rotulo",endereco.rotulo);
	fprintf(f,"%-30s: %d bits\n","Conjunto",endereco.conjunto);
	fprintf(f,"%-30s: %d bits\n\n","Palavra",endereco.palavra);
	fprintf(f,"TOTAL DE ENDEREÇOS NO ARQUIVO DE ENTRADA\n--------------------------------------------------------\n");
	fprintf(f,"%-30s: %.0f\n","Leituras", cache->leituras);
	fprintf(f,"%-30s: %0.f\n","Escritas", cache->escritas);
	fprintf(f,"%-30s: %0.f\n\n","Total de leituras e escritas", cache->escritas + cache->leituras);
	fprintf(f,"TOTAL DE ESCRITAS/LEITURAS NA MEMORIA PRINCIPAL\n--------------------------------------------------------\n");
	fprintf(f,"%-30s: %0.f\n","Leituras na MP", cache->leituraNaMP);
	fprintf(f,"%-30s: %0.f\n","Escritas na MP", cache->escritaNaMP);
	fprintf(f,"%-30s: %0.f\n\n","Leituras + Escritas na MP", cache->escritaNaMP + cache->leituraNaMP);
	fprintf(f,"TOTAL DE LEITURAS/ESCRITAS NA CACHE\n--------------------------------------------------------\n");
	fprintf(f,"%-30s: %0.f\n","Leitura", cache->leituraCache);
	fprintf(f,"%-30s: %0.f\n\n","Escrita", cache->escritaCache);
	fprintf(f,"TAXA DE ACERTO\n--------------------------------------------------------\n");
	fprintf(f,"%-30s: %f %%\n","Taxa de acerto de Leitura", (cache->leituraCache/cache->leituras)*100);
	fprintf(f,"%-30s: %f %%\n","Taxa de acerto de Escrita", (cache->escritaCache/cache->escritas)*100);
	fprintf(f,"%-30s: %f %%\n\n","Taxa de acerto de Global", ((cache->escritaCache + cache->leituraCache)/(cache->leituras + cache->escritas)) * 100);
	fprintf(f,"TEMPO MEDIO DE ACESSO\n--------------------------------------------------------\n");
	fprintf(f,"%-30s: %f ns\n", "Tempo medio:", cache->tempoMedio);
	
}
int main(){
	CACHE *cache = setarCache();
	FITA *fita = carregar();
	int i = 0;	
	for(i = 0; i < fita->tamanho; i++)
		lerCache(cache,fita->endereco[i],fita->tipoAcesso[i]);
	tempo(cache);
	relatorioCache(cache);
}
