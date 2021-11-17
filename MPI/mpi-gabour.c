#include <opencv2/core/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <omp.h>
#include <iostream>
#include <stdio.h>
#include <sys/time.h>


int main(int argc, char** argv) {


    Mat imagem;

    imagem = imread(argv[1],IMREAD_GRAYSCALE);
    if(!imagem.data)
        cout << "nao abriu imagem" << endl;

    int linhas, colunas;
    linhas = imagem.rows;
    colunas = imagem.cols;
    //resultado, os valores dos pixels serão substituidos pelo resultado final no decorrer do processo

    Mat kernel = getGaborKernel(cv::Size(kernel_size,kernel_size), sigma, theta, lamda, gamma, phi, CV_32FC1);

    double imagem_filtrada[linhas][7];

    int kernel_size = 5;
    int offset_kernel = (kernel_size-1)/2;
    double sigma , theta , lamda , gamma, phi;

    sigma = 5;
    theta = 0;
    lamda = PI/4;
    gamma = 0.1;
    phi = 0;    

    int x_img, y_img;
    int i, j;
    int count_x, count_y;
    double sum;
    int x_kernel, y_kernel, offset_kernel;

    int  meu_id;                                      //identificador do processo/
    int  numero_processos;                            //numero de processos/
    int  origem;                                      //rank de quem envia/
    int  destino;                                     //rank de quem recebe/
    int  tag=0;                                       //identificador de mensagens/
    int mensagem_send[10];                            //mensagem (integer))/
    char nome_host [MPI_MAX_PROCESSOR_NAME];          //Comprimento maximo do nome do host retornado por MPI_Get_processor_name/
    int  tamanho_nome;                                //Comprimento (em caracteres) do nome/     
    int contador;                                     //contador/                                   

    double *imagem_parcial;
    int linha_start, linha_end;

    int linha_img;

    int equal_tasks;
    int rcv_size;


    int tarefa_qtd, tarefa_sobra; 
    int aux_size; //para definir o tamanho da imagem filtrada




    offset_kernel = 1 ;//(ordem_filtro - 1)/2

    MPI_Status status;

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

    tarefa_qtd = linhas/(numero_processos - 1);
    tarefa_sobra = linhas/(numero_processos - 1) + linhas % (numero_processos - 1);

///////////////////////////////////////////////////////////////////////////
//////////////////////////////// RANK = 0 /////////////////////////////////
///////////////////////////////////////////////////////////////////////////

    if(meu_id==0) { //se eu sou o root/
    
        if (linhas % (numero_processos - 1) == 0){
            equal_tasks = 1;
        }
        else equal_tasks = 0;

        printf("\nMestre:%s  Processo %d\n", nome_host,meu_id);

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

            printf("RANK: %d -> start: %d, end: %d\n",destino, linha_start, linha_end );
            MPI_Send(mensagem_send,10,MPI_INT,destino,tag,MPI_COMM_WORLD);
        }

        //recebimento das tarefas
        if(equal_tasks) {
            // se forem de tamanhos iguais o vetor que recebe será sempre do mesmo tamanho uma vez que todos os processos
            // computam a mesma quantidade de linhas
            rcv_size = tarefa_qtd*colunas;
            imagem_parcial = malloc(rcv_size*sizeof(double));
            linha_img = 0;
            for(destino=1;destino<numero_processos;destino++){
                MPI_Recv(imagem_parcial,rcv_size, MPI_DOUBLE, destino,tag,MPI_COMM_WORLD, &status);
                //Dentro desse for que deve ser passado o valor do vetor para a imagem filtrada.
               
                //linha img é a linha que vai salvar na imagem
                for(i= 0; i<rcv_size;i++ ){
                    if(i % colunas == 0 && i != 0){
                        linha_img++;
                    }
                    imagem_filtrada[linha_img][i % colunas] = imagem_parcial[i];
                }
                linha_img++;   
            }
        }
        else{
            // Se nao forem cargas de trabalhos iguais entao o ultimo processo ficará com carga a mais, por esse motivo ele deve ser tratado diferente
            rcv_size = tarefa_qtd*colunas;
            imagem_parcial = malloc(rcv_size*sizeof(double));
            linha_img = 0;
            for(destino=1;destino<numero_processos - 1;destino++){
                MPI_Recv(imagem_parcial,rcv_size, MPI_DOUBLE, destino,tag,MPI_COMM_WORLD, &status);
                //Dentro desse for que deve ser passado o valor do vetor para a imagem filtrada.
                for(i= 0; i<rcv_size;i++ ){
                    if(i % colunas == 0 && i != 0){
                        linha_img++;
                    }

                    imagem_filtrada[linha_img][i % colunas] = imagem_parcial[i];
                linha_img++;   
            }
            // define o tamanho diferente para o ultimo processo
            rcv_size = tarefa_sobra*colunas;
            printf("\n\n\nrcv_size: %d\n\n\n", rcv_size);
            free(imagem_parcial);
            imagem_parcial = malloc(rcv_size*sizeof(double));
            MPI_Recv(imagem_parcial,rcv_size, MPI_DOUBLE, numero_processos - 1,tag,MPI_COMM_WORLD, &status);
            //Dentro desse for que deve ser passado o valor do vetor para a imagem filtrada.
            for(i= 0; i<rcv_size;i++ ){
                if(i % colunas == 0 && i!=0 ){
                        linha_img++;
                    }

                imagem_filtrada[linha_img][i % colunas] = imagem_parcial[i];
            }   

        }

        for(i= 0; i<linhas;i++ ){
            for(j= 0; j<colunas;j++ ){
                printf("RANK %d-> Valor[%d][%d] = %f\n", meu_id ,i,j, imagem_filtrada[i][j]);
            }
        }
    }

///////////////////////////////////////////////////////////////////////////
//////////////////////////////// RANK != 0 ////////////////////////////////
///////////////////////////////////////////////////////////////////////////
else{

    MPI_Recv(mensagem_send,10,MPI_INT, 0,tag,MPI_COMM_WORLD, &status);   //Recebe mensagem/
    // printf("Worker: %d -> start: %d, end: %d\n",meu_id, mensagem_send[0], mensagem_send[1] );

    aux_size = mensagem_send[1] - mensagem_send[0];

    imagem_parcial = malloc((aux_size*colunas)*sizeof(double));
    int qtde = aux_size*colunas;

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
                            sum += kernel.at<float>(count_x,count_y)*(int)imagem.at<uchar>(x_kernel,y_kernel);
                        }
                        count_y++;
                    }
                    count_x++;
                }
                // ajeitar esses indices
                imagem_parcial[(i-mensagem_send[0])*colunas + j] = sum;
            }
    }

        MPI_Send(imagem_parcial,aux_size*colunas,MPI_DOUBLE,0,tag,MPI_COMM_WORLD);

    
    
}
return 0;

}
