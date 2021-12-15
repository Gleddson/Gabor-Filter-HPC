#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/opencv.hpp>

#include <stdio.h>
#include <stdlib.h>

#include <CL/cl.h>

#include <iostream>
#include<fstream>

#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include<fcntl.h>

#include<iostream>

using namespace std;
using namespace cv;

void LoadImageFromOpenCV(cl_context& context, cv::Mat image, cl_mem& imageObject, size_t height, size_t width, cl_int& err) {
    
    uchar* data = image.data;

    cl_image_format clImageFormat;
    clImageFormat.image_channel_order = CL_INTENSITY;
    clImageFormat.image_channel_data_type = CL_UNSIGNED_INT8;

    cl_image_desc desc;
    desc.image_type = CL_MEM_OBJECT_IMAGE2D;
    desc.image_width = width;
    desc.image_height = height;

    desc.image_depth = 1;
    desc.image_array_size = 0;
    desc.image_row_pitch = 0;
    desc.image_slice_pitch = 0;
    desc.num_mip_levels = 0;
    desc.num_samples = 0;
    desc.mem_object = 0;

    imageObject = clCreateImage(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, &clImageFormat, &desc, data, &err);

}

void Cleanup(cl_context context, cl_command_queue commandQueue, cl_program program, cl_kernel kernel, cl_mem images[2], cl_sampler sampler) {
    
    for (int i = 0; i < 2; i++) {
        if (images[i] != 0) {
            clReleaseMemObject(images[i]);
        }
    }
    if (commandQueue != 0) {
        clReleaseCommandQueue(commandQueue);
    }
    if (kernel != 0) {
        clReleaseKernel(kernel);
    }
    if (program != 0) {
        clReleaseProgram(program);
    }
    if (sampler != 0) {
        clReleaseSampler(sampler);
    }
    if (context != 0) {
        clReleaseContext(context);
    }
}

cl_program CreateProgram(cl_context context, cl_device_id device, const char* fileName) {
    cl_int err;
    cl_program program;

    std::ifstream kernelFile(fileName, std::ios::in);

    if (!kernelFile.is_open())
    {
        std::cerr << "Failed to open file for reading: " << fileName << std::endl;
        return NULL;
    }

    std::ostringstream oss;
    oss << kernelFile.rdbuf();

    std::string srcStdStr = oss.str();
    const char* srcStr = srcStdStr.c_str();
    program = clCreateProgramWithSource(context, 1, (const char**)&srcStr, NULL, NULL);

    if (program == NULL)
    {
        std::cerr << "Failed to create CL program from source." << std::endl;
        return NULL;
    }

    err = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
    if (err != CL_SUCCESS)
    {
  
        char buildLog[16384];
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, sizeof(buildLog), buildLog, NULL);

        std::cerr << "Error in kernel: " << std::endl;
        std::cerr << buildLog;
        clReleaseProgram(program);
        return NULL;
    }

    return program;
}

cl_context CreateContext(cl_int& err) {
    
    cl_platform_id *platform;
    cl_uint numplatform;
    cl_context context = NULL;

    err = clGetPlatformIDs(0, NULL, &numplatform);
    platform = (cl_platform_id*)malloc(sizeof(cl_platform_id) * numplatform);
    err = clGetPlatformIDs(numplatform, platform, NULL);
    
    if (err != CL_SUCCESS || numplatform <= 0)
    {
        std::cerr << "Failed to find any OpenCL platforms." << std::endl;
        return NULL;
    }
    cl_context_properties contextProperties[] = {
                                                    CL_CONTEXT_PLATFORM,
                                                    (cl_context_properties) platform[0],
                                                    0
                                                };

    context = clCreateContextFromType(contextProperties, CL_DEVICE_TYPE_GPU, NULL, NULL, &err);
    
    if (err != CL_SUCCESS)
    {
        std::cout << "Could not create GPU context, trying CPU..." << std::endl;
        context = clCreateContextFromType(contextProperties, CL_DEVICE_TYPE_CPU,
            NULL, NULL, &err);
        if (err != CL_SUCCESS)
        {
            std::cerr << "Failed to create an OpenCL GPU or CPU context." << std::endl;
            return NULL;
        }
    }

    return context;

}

cl_command_queue CraeteCommandQueue(cl_context context, cl_device_id& device) {
    cl_int err;
    cl_device_id* devices;
    cl_command_queue commandQueue = NULL;
    size_t numDevices = -1;
    err = clGetContextInfo(context, CL_CONTEXT_DEVICES, 0, NULL, &numDevices);

    if (err != CL_SUCCESS) {
        std::cerr << "Failed get device information.";
        return NULL;
    }
    if (numDevices <= 0) {
        std::cerr << "No devices available.";
        return NULL;
    }
    devices = new cl_device_id[numDevices / sizeof(cl_device_id)];
    err = clGetContextInfo(context, CL_CONTEXT_DEVICES, numDevices, devices, NULL);

    if (err != CL_SUCCESS) {
        std::cerr << "Fail to get device IDs";
        return NULL;
    }
    commandQueue = clCreateCommandQueueWithProperties(context, devices[0], NULL, &err);
    if (err != CL_SUCCESS) {
        std::cerr << "Fail to create command queue.";
        return NULL;
    }
    device = devices[0];
    delete[] devices;
    return commandQueue;
}

int main()
{   
    //Declaração das variáveis
    size_t height = 0, width = 0;
    Mat img, grayimage;

    clock_t start;
    double tempo_execucao = 0, tempo_transferencia = 0, tempo_computo = 0;

    //Abringo imagem
    //img = imread("lena.png");
	//img = imread("minion.jpg");
	//img = imread("C:/Users/Gledson/Desktop/minion.jpg");
    img = imread("Teclado.jpg");
    //img = imread("drag.png");

	if (img.empty())
	{
		cout << "Could not open or find the image" << endl;
		cin.get(); //wait for any key press
		return -1;
	}

    //Exibindo a imagem na tela.
	//namedWindow("image", WINDOW_NORMAL);
	//imshow("image", img);
	//waitKey(0);

    height = img.rows;
    width = img.cols;

    printf("Tamanho da imagem: \n");
    printf("height da imagem: %d \n", height);
    printf("width da imagem: %d \n \n", width);


    if (!img.data)
    {
        std::cerr << "Fail to load image!" << endl;
        return -1;
    }
    
    cvtColor(img, grayimage, COLOR_BGR2GRAY);

    float  filter[3][3] = {
        {-1, 0, -1},
        { 0, 4,  0},
        {-1, 0, -1}
    };

    //imshow("image", grayimage);
    //waitKey(0);

    for (int i = 0; i < 100; i++) {

        /*Init Platform*/
        cl_context context = 0;
        cl_command_queue commandQueue = 0;
        cl_program program = 0;
        cl_device_id device = 0;
        cl_kernel kernel = 0;
        cl_mem images[2] = { 0,0 };
        cl_sampler sampler = 0;
        cl_int err;
        cl_uint num_platforms;

        err = clGetPlatformIDs(0, NULL, &num_platforms);

        if (num_platforms == 0)
        {
            printf("Found 0 platforms!\n");
            return EXIT_FAILURE;
        }
        //printf("1 - Nmero de plataformas: %i \n", num_platforms);

        //Criando o Contexto
        context = CreateContext(err);

        if (err != CL_SUCCESS) {
            printf("Failed to create OpenCL context. \n");
            return EXIT_FAILURE;
        }
        //printf("2 - OpenCL context criado \n");

        //Criando a  a Command Queue 
        commandQueue = CraeteCommandQueue(context, device);

        if (commandQueue == NULL) {
            Cleanup(context, commandQueue, program, kernel, images, sampler);
            return -1;
        }
        //printf("3 - Command Queue  criada \n");

        //Output device information
        cl_char vendor_name[100] = { 0 };
        cl_char device_name[100] = { 0 };
        size_t returned_size = 0;

        err = clGetDeviceInfo(device, CL_DEVICE_VENDOR, sizeof(vendor_name), vendor_name, &returned_size);
        err |= clGetDeviceInfo(device, CL_DEVICE_NAME, sizeof(device_name), device_name, &returned_size);

        if (err != CL_SUCCESS) {
            return -1;
        }
        //printf("4 - Conectando ao Device %s %s...\n", vendor_name, device_name);

        //Testando se o Device suporta imagem
        cl_bool imageSupport = CL_FALSE;
        clGetDeviceInfo(device, CL_DEVICE_IMAGE_SUPPORT, sizeof(cl_bool), &imageSupport, NULL);

        if (imageSupport != CL_TRUE) {
            printf("OpenCL device nao suporta imagem. \n");
            Cleanup(context, commandQueue, program, kernel, images, sampler);
            return -1;
        }
        //printf("5 - OpenCL device suporta imagem. \n");

        LoadImageFromOpenCV(context, grayimage, images[0], height, width, err);
    
        if (err != CL_SUCCESS) {
            printf("Erro ao criar um objeto iamgem. \n");
            Cleanup(context, commandQueue, program, kernel, images, sampler);
            return -1;
        }
        //printf("6 - Imagem Criada com sucesso. \n");

        // Create objeto umagem de saída
        cl_image_format clImageFormat;
        clImageFormat.image_channel_order = CL_INTENSITY;
        clImageFormat.image_channel_data_type = CL_UNSIGNED_INT8;

        cl_image_desc desc;
        desc.image_type = CL_MEM_OBJECT_IMAGE2D;
        desc.image_width = width;
        desc.image_height = height;

        desc.image_depth = 1;
        desc.image_array_size = 0;
        desc.image_row_pitch = 0;
        desc.image_slice_pitch = 0;
        desc.num_mip_levels = 0;
        desc.num_samples = 0;
        desc.mem_object = 0;

        images[1] = clCreateImage(context, CL_MEM_WRITE_ONLY, &clImageFormat, &desc, NULL, &err);

        if (err != CL_SUCCESS)
        {
            printf("6 - Erro ao criar um objeto iamgem de saída. \n");
            Cleanup(context, commandQueue, program, kernel, images, sampler);
            return -1;
        }
        //printf("6 - Imagem de saida criada com sucesso. \n");

        //Create image sampler
        sampler = clCreateSampler(context, CL_FALSE,  CL_ADDRESS_CLAMP_TO_EDGE, CL_FILTER_LINEAR, &err);
    
        if (err != CL_SUCCESS)
        {
            printf("  - Error creating sampler object. \n");
            Cleanup(context, commandQueue, program, kernel, images, sampler);
            return -1;
        }
        //printf("7 - Sampler object criado com sucesso. \n");

        // -------------------------------------------------------------------------------------------

        //Criando programa
        program = CreateProgram(context, device, "ImageFilter2D.cl");

        if (program == NULL)
        {
            printf("  - Erro ao criar o programa. \n");
            Cleanup(context, commandQueue, program, kernel, images, sampler);
            return -1;
        }
        //printf("8 - Programa criado com sucesso. \n");

        // Criando OpenCL kernel
        kernel = clCreateKernel(program, "edge_filter", NULL);

        if (kernel == NULL)
        {
            printf("  - Erro ao criar o Kernel. \n");
            Cleanup(context, commandQueue, program, kernel, images, sampler);
            return -1;
        }
        //printf("9 - Kernel criado com sucesso. \n");

        //Set the kernel arguments
        err = clSetKernelArg(kernel, 0, sizeof(cl_mem), &images[0]);
        err |= clSetKernelArg(kernel, 1, sizeof(cl_mem), &images[1]);
        err |= clSetKernelArg(kernel, 2, sizeof(cl_sampler), &sampler);
        err |= clSetKernelArg(kernel, 3, sizeof(cl_int), &width);
        err |= clSetKernelArg(kernel, 4, sizeof(cl_int), &height);

        if (err != CL_SUCCESS)
        {
            printf("  - Erro na definição dos argumentos do Kernel. \n");
            Cleanup(context, commandQueue, program, kernel, images, sampler);
            return -1;
        }
        //printf("10 - Argumentos do kernel definidos com sucesso. \n");

        /*
    
        work_dims—The number of dimensions in the data
         - global_work_size — The number of work items in each dimension
         - local_work_size — The number of work items in a workgroup, in each dimension
    
        */

        //Set workGroup
        size_t localWorkSize[2] = { 1000, 1000 };
        size_t globalWorkSize[2] = { width, height };

        //printf("11 - Workgroup criado com sucesso. \n");

        // Execução do Programa
        start = std::clock();

        //err = clEnqueueNDRangeKernel(commandQueue, kernel, 2, NULL, globalWorkSize, localWorkSize, 0, NULL, NULL);
        err = clEnqueueNDRangeKernel(commandQueue, kernel, 2, NULL, globalWorkSize, NULL, 0, NULL, NULL);

        if (err != CL_SUCCESS) {
            printf("   - Erro na execução do Kernel. \n");
            Cleanup(context, commandQueue, program, kernel, images, sampler);
            return -1;
        }

        //printf("12 - Kernel Executado com sucesso. \n");

        double tempo_computo_aux = (std::clock() - start) / (double)CLOCKS_PER_SEC;
        tempo_computo = tempo_computo  + tempo_computo_aux;
        //std::cout << "\nTempo de Computo aux: " << tempo_computo_aux << std::endl;
        //std::cout << "\nTempo de Computo: " << tempo_computo << std::endl;

        //------------------------------------------------------------------------

        uchar* buffer = new uchar[width * height];
        size_t origin[3] = { 0, 0, 0 };
        size_t region[3] = { width, height, 1 };

        err = clEnqueueReadImage(commandQueue, images[1], CL_TRUE, origin, region, 0, 0, buffer, 0, NULL, NULL);

        if (err != CL_SUCCESS)
        {
            printf("%i - ", err);
            std::cerr << "Error reading result buffer." << std::endl;
            Cleanup(context, commandQueue, program, kernel, images, sampler);
            return -1;
        }

        double tempo_execucao_aux = (std::clock() - start) / (double) CLOCKS_PER_SEC;
        tempo_execucao = tempo_execucao + tempo_execucao_aux;

        double tempo_transferencia_aux = tempo_execucao_aux - tempo_computo_aux;
        tempo_transferencia = tempo_transferencia + tempo_transferencia_aux;

        //std::cout << "\nTempo de Transferencia: " << tempo_transferencia << std::endl;

        //std::cout << "\nTempo de Execucao: " << tempo_execucao << std::endl;

        Mat output(height, width, CV_8U, buffer);
        imwrite("result.png", output);

        delete[]buffer;

        Cleanup(context, commandQueue, program, kernel, images, sampler);
    }

    std::cout << "\nTempo de Computo: " << tempo_computo << std::endl;

    std::cout << "\nTempo de Transferencia: " << tempo_transferencia << std::endl;

    std::cout << "\nTempo de Execucao: " << tempo_execucao << std::endl;
   
    return 0;
}
