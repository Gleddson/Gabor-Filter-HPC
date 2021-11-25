#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>
#include "mpi.h"

#define linhas 7620
#define colunas 5020
#define k_size 5


double startTimeHost, startTimeW , finalTimeHost, finalTimeW ;
time_t inicioHost, fimHost, inicioWorker, fimWorker;
char buffer1[25], buffer2[25];
struct tm* tm_info;

double getRealTime(){
    struct timeval tm;
    gettimeofday(&tm, NULL);
    return ((double)tm.tv_sec + (double)tm.tv_usec/1000000.0);
}

void inicializarImagem(double *img){

    int i;
    for(i=0; i < linhas*colunas; i++) {
           img[i] = i*0.007 - i*0.003;
        //    printf("img[%d][%d] = %f \n", i, j,  img[i][j]);
    }       
}


int main(int argc, char** argv) {

    double *imagem;
    double *imagem_filtrada;
    
    imagem = (double*)malloc(linhas*colunas*sizeof(double));


    imagem_filtrada = (double*)malloc(linhas*colunas*sizeof(double));
    inicializarImagem(imagem);


    double kernel[k_size][k_size] = {{1.0, -1.0, -1.9143, .02, 1.4},
                          {1.32, .64, -1.0, -1.9143, 0.860},
                          {.64, -1.0, 1.09, 0.41, 3.0},
                          {1.0, -1.0, -1.9143, .02, 1.4},
                          {.64, -1.0, 1.09, 0.41, 3.0}};


    int i, j;
    int count_x, count_y;
    double sum;
    int x_kernel, y_kernel, offset_kernel;

    offset_kernel = (k_size - 1)/2;

    int  meu_id;                                      //identificador do processo/
    int  numero_processos;                            //numero de processos/
    int  origem;                                      //rank de quem envia/
    int  destino;                                     //rank de quem recebe/
    int  tag=0;                                       //identificador de mensagens/
    int mensagem_send[2];                         //mensagem (integer))/
    char nome_host [MPI_MAX_PROCESSOR_NAME];          //Comprimento maximo do nome do host retornado por MPI_Get_processor_name/
    int  tamanho_nome;                                //Comprimento (em caracteres) do nome/                                       

    double *imagem_parcial;
    int linha_start, linha_end;

    int linha_img;

    int equal_tasks;
    int rcv_size;

    int tarefa_qtd, tarefa_sobra; 
    int aux_size; //para definir o tamanho da imagem filtrada



    //Inicializa MPI/
    MPI_Init(&argc, &argv);
    // /Descobre quem sou eu -- rank/
    MPI_Comm_rank(MPI_COMM_WORLD,&meu_id);
    //Descobre quanto processos existem/
    MPI_Comm_size(MPI_COMM_WORLD,&numero_processos);
    //Descobre meu nome/
    MPI_Get_processor_name(nome_host,&tamanho_nome);
    
    // printf("executando...\n");
    // printf("RANK: %d \n",meu_id );

    // Tempo início


    tarefa_qtd = linhas/(numero_processos - 1);
    tarefa_sobra = linhas/(numero_processos - 1) + linhas % (numero_processos - 1);

///////////////////////////////////////////////////////////////////////////
//////////////////////////////// RANK = 0 /////////////////////////////////
///////////////////////////////////////////////////////////////////////////

    if(meu_id==0) { //se eu sou o root/
    
    time(&inicioHost);
    tm_info = localtime(&inicioHost);
    strftime(buffer1, 25, "%d/%m/%Y %H:%M:%S", tm_info);
    startTimeHost = getRealTime(); 

    MPI_Status status;

        // Verifica se a quantidade de tarefas vai ser distribuida igualmente ou se algum worker ficará com mais tarefas
        if (linhas % (numero_processos - 1) == 0){
            equal_tasks = 1;
        }
        else equal_tasks = 0;

        // Distribuição das tarefas que serão atribuídas para cada worker
        for(destino=1;destino<numero_processos;destino++){  //Inicio do for/
            
            if(equal_tasks) {
                linha_start = (destino - 1)* tarefa_qtd;
                linha_end = linha_start + tarefa_qtd;
            } else {
                if(destino == (numero_processos - 1)) {
                    linha_start = linha_end;
                    linha_end = linhas;
                } else {
                    linha_start = (destino - 1)* tarefa_qtd;
                    linha_end = linha_start + tarefa_qtd;
                }
            }

            mensagem_send[0]=linha_start;
            mensagem_send[1]=linha_end;
            // Envio da mensagem com os valores definindo a linha de inicio e de fim do trabalho de cada worker
            MPI_Send(mensagem_send,2,MPI_INT,destino,tag,MPI_COMM_WORLD);
        }

        //recebimento das tarefas
        if(equal_tasks) {
            // se forem de tamanhos iguais o vetor que recebe será sempre do mesmo tamanho uma vez que todos os processos
            // computam a mesma quantidade de linhas
            rcv_size = tarefa_qtd*colunas;
            imagem_parcial = (double*) malloc(rcv_size*sizeof(double));
            linha_img = 0;

            for(destino=1;destino<numero_processos;destino++){
                MPI_Recv(imagem_parcial,rcv_size, MPI_DOUBLE, destino,tag,MPI_COMM_WORLD, &status);
                //Dentro desse for que deve ser passado o valor do vetor para a imagem filtrada.
                for(i= 0; i<rcv_size;i++ ){
                     if(i % colunas == 0 && i != 0){
                        linha_img++;
                    }
                    imagem_filtrada[linha_img*colunas + i % colunas] = imagem_parcial[i];
                                      
                }
                // printf("linha_img: %d\n\n", linha_img); 
                linha_img++;
            }
        }

        else{
            // Se nao forem cargas de trabalhos iguais entao o ultimo processo ficará com carga a mais, por esse motivo ele deve ser tratado diferente
            rcv_size = tarefa_qtd*colunas;
            imagem_parcial = (double*)malloc(rcv_size*sizeof(double));
            linha_img = 0;
            for(destino=1;destino<numero_processos - 1;destino++){
                MPI_Recv(imagem_parcial,rcv_size, MPI_DOUBLE, destino,tag,MPI_COMM_WORLD, &status);
                //Dentro desse for que deve ser passado o valor do vetor para a imagem filtrada.
                for(i= 0; i<rcv_size;i++ ){
                    if(i % colunas == 0 && i != 0){
                        linha_img++;
                    }
                    imagem_filtrada[linha_img*colunas + i % colunas] = imagem_parcial[i];
                } 
                linha_img++;
            }
            // define o tamanho diferente para o ultimo processo
            rcv_size = tarefa_sobra*colunas;
            free(imagem_parcial);
            imagem_parcial = (double*) malloc(rcv_size*sizeof(double));
            MPI_Recv(imagem_parcial,rcv_size, MPI_DOUBLE, numero_processos - 1,tag,MPI_COMM_WORLD, &status);
            //Dentro desse for que deve ser passado o valor do vetor para a imagem filtrada.
            for(i= 0; i<rcv_size;i++ ){
                if(i % colunas == 0 && i!=0 ){
                        linha_img++;
                    }
                imagem_filtrada[linha_img*colunas + i % colunas] = imagem_parcial[i];

            }   

        }
        
    }

///////////////////////////////////////////////////////////////////////////
//////////////////////////////// RANK != 0 ////////////////////////////////
///////////////////////////////////////////////////////////////////////////
else{

    time(&inicioWorker);
    tm_info = localtime(&inicioWorker);
    strftime(buffer1, 25, "%d/%m/%Y %H:%M:%S", tm_info);
    startTimeW = getRealTime(); 

    MPI_Status status;

    MPI_Recv(mensagem_send,2,MPI_INT, 0,tag,MPI_COMM_WORLD, &status);   //Recebe mensagem/

    aux_size = mensagem_send[1] - mensagem_send[0];
    // printf("Tamanho imagem/RANK %d: %d e %d\n",meu_id, aux_size, colunas);
    imagem_parcial = (double*) malloc((aux_size*colunas)*sizeof(double));
    int qtde = aux_size*colunas;

    // Aplicação do filtro de Gabour nas partes da imagem atribuidas como tarefa para worker que está executando
    for(i=mensagem_send[0]; i<mensagem_send[1]; i++){        
            for(j=0;j<colunas;j++){
                sum = 0;
                count_x = 0;

                // Convolução do kernel
                for(x_kernel= i-offset_kernel; x_kernel<= i+offset_kernel; x_kernel++){
                    count_y = 0;

                    for(y_kernel= j-offset_kernel; y_kernel<= j+offset_kernel; y_kernel++){        
                        if(y_kernel >= 0 && x_kernel >= 0 && y_kernel < colunas && x_kernel < linhas){
                            //Multiplicaçao entre pixel e kernel; 
                            sum += kernel[count_x][count_y]*imagem[x_kernel*colunas + y_kernel];
                        }
                        count_y++;
                    }
                    count_x++;
                }

                // Atribuição dos resultados ao vetor de resultados parciais daquele worker
                imagem_parcial[(i-mensagem_send[0])*colunas + j] = sum;
            }
    }

        time(&fimWorker);
        tm_info = localtime(&fimWorker);
        strftime(buffer2, 25, "%d/%m/%Y %H:%M:%S", tm_info);
        finalTimeW = getRealTime();

        MPI_Send(imagem_parcial,aux_size*colunas,MPI_DOUBLE,0,tag,MPI_COMM_WORLD);

        printf("Tempo CPU rank %d: %f s\n", meu_id, finalTimeW-startTimeW);
    
}

time(&fimHost);
tm_info = localtime(&fimHost);
strftime(buffer2, 25, "%d/%m/%Y %H:%M:%S", tm_info);
finalTimeHost = getRealTime();
if(meu_id==0)
    printf("\nTempo de Execucao: %f segundos\n",finalTimeHost-startTimeHost);


MPI_Finalize();
return 0;

}

