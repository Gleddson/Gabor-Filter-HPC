#include <opencv2/core/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <omp.h>
#include <iostream>
#include <stdio.h>
#include <sys/time.h>

#define PI 3.14159265359

using namespace std;
using namespace cv;


int main(int argc, char* argv[]){ 

    Mat imagem;
 
    //configurações do kernel gabour
    int kernel_size = 5;
    double sigma , theta , lamda , gamma, phi;

    int offset_kernel;
    int sum, count_x, count_y, x_img, y_img;
    int y_kernel, x_kernel;
    int i, j;

    int num_cores;
    
    num_cores = atoi(argv[2]);

    sigma = 5;
    theta = 0;
    lamda = PI/4;
    gamma = 0.1;
    phi = 0;

    offset_kernel = (kernel_size-1)/2;

    //imagem a ser filtrada
    imagem = imread(argv[1],IMREAD_GRAYSCALE);
    if(!imagem.data)
        cout << "nao abriu imagem" << endl;

    //resultado, os valores dos pixels serão substituidos pelo resultado final no decorrer do processo
    Mat imagem_filtrada = imread(argv[1],IMREAD_GRAYSCALE);

    //Tamanho da imagem
    x_img = imagem.rows;
    y_img = imagem.cols;


    //Criação do Kernel de Gabour
    Mat kernel = getGaborKernel(cv::Size(kernel_size,kernel_size), sigma, theta, lamda, gamma, phi, CV_32FC1); 

    // Percorre os pixels da imagem
    #pragma omp parallel for private(x_kernel, y_kernel,i,  j,count_x, count_y,sum) num_threads(num_cores)
    for(i=0;i<x_img;i++){
        
        for(j=0;j<y_img;j++){
            sum = 0;
            count_x = 0;

            // Convolução do kernel
            for(x_kernel= i-offset_kernel; x_kernel< i+offset_kernel; x_kernel++){
                count_y = 0;
                for(y_kernel= j-offset_kernel; y_kernel< j+offset_kernel; y_kernel++){
                    
                    if(y_kernel >= 0 && x_kernel >= 0 && y_kernel < y_img && x_kernel < x_img){

                        //Multiplicaçao entre pixel e kernel
                         sum += kernel.at<float>(count_x,count_y)*(int)imagem.at<uchar>(x_kernel,y_kernel);
                    }
                    
                    count_y++;
                }
                count_x++;   


            }

            imagem_filtrada.at<uchar>(i,j) = sum;

        }
    }

    bool check = imwrite("./resultado.jpg", imagem_filtrada);

    if(check){
        printf("Salvo com sucesso!");
    }    


    return 0;
    }