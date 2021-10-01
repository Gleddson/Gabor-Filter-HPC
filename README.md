# Gabor-Filter-HPC

## Autores 

Gledson de Oliveira [@gleddson].

Gustavo Coelho [@GustavoAACoelho].

## Sobre

A implementação do filtro de Gabour na linguagem C/C++ foi realizada a partir do auxilio da biblioteca Opencv. O objetivo principal do uso da biblioteca foi a facilidade apresentada por ela para obtenção e manipulação de pixels de imagens. A implementação como um todo consiste na definição de um filtro de Gabour, que foi criado a partir da utilização de funções do opencv. A partir disso o filtro é apicado na imagem, pixel por pixel. 

Para as situações que o filtro está na borda da imagem, foi considerado que as posições excedentesda imagem possuem valor zero.


Para executar o código é necessário instalar o Opencv, de preferência utilizando o cmake (segue link do tutorial https://medium.com/@carvalho.natalia03/instalando-a-opencv-c-no-linux-98d7fc71e996). A compilação do código é feita a partir do cmake, utilizando o arquivo CMakeListpara gerar o executável. Essa abordagem permite a fácil a compilaçao do código utilizando a biblioteca Opencv e openmp.

No momento de executar o código é necessário passar como parâmetro na linha de comando o nome da imagem que se deseja aplicar o filtro e a quantidade de threads que se deseja utilizar no paralelismo.

A imagem a seguir representa um exemplo de saída do Filtro produzido neste trabalho

![Screenshot](Saída_Filtro_de_Gabor.png)


## Execução em Serial

Para a execução em serial é necessário:

* dentro de CMakeList, substituir 'nomearquivo.cpp' pelo arquivo desejado.
* dentro de CMakeList, substituir 'executavel' pelo nome do que se deseja para o arquivo executável

* executar $ cmake .
* executar $ make
* executar $./nome_do_exectavel nome_imagem

## Execução em Paralelo Utilizando OpenMP

Para a execução em paralelo é necessário:

* dentro de CMakeList, substituir 'nomearquivo.cpp' pelo arquivo desejado.
* dentro de CMakeList, substituir 'executavel' pelo nome do que se deseja para o arquivo executável

* executar $ cmake .
* executar $ make
* executar $./nome_do_exectavel nome_imagem num_threads
