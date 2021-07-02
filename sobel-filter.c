#define _CRT_SECURE_NO_WARNINGS
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include <omp.h>

#define NUM_THREADS 64

unsigned char * readPGM(const char * sourcefile, int * width, int * height);
int 			writePGM(const char * filepath, unsigned char * image, int width, int height);

void reduce_noise  (unsigned char * image, const int width, const int height);
double edge_detection_seq(unsigned char * image, unsigned char * image_edges, const int width, const int height);
double edge_detection_omp(unsigned char * image, unsigned char * image_edges, const int width, const int height);


int main(void) {

    const char source [] = "Testbilder/Testbild_2.pgm";

    const char target_seq[] = "Ergebnisse/sobel_seq.pgm";
    const char target_omp[] = "Ergebnisse/sobel_omp.pgm";

    // execution times
    double t_seq, t_omp;

    // init values
    unsigned char *image = NULL;
    int width, height;

    // read file
    image = readPGM(source, &width, &height);
    if (image == NULL){
        printf("Error! Could not read file!\n");
        return 1;
    }

    // create buffer to store modified image
    unsigned char *image_edges = NULL;
    image_edges = (unsigned char*) malloc(width * height * sizeof(unsigned char));
    if (image_edges == NULL){
        printf("Error! Could not allocate memory for results!\n");
        return 1;
    }

    reduce_noise  (image, width, height);

    t_seq = edge_detection_seq(image, image_edges, width, height);
    writePGM("Ergebnisse/testedges_seq.pgm", image_edges, width, height);
    t_omp = edge_detection_omp(image, image_edges, width, height);
    writePGM("Ergebnisse/testedges_omp.pgm", image_edges, width, height);

    printf("\n");
    printf("Soble filter execution times:\n");
    printf("t_seq: \t%f\n", t_seq);
    printf("t_omp: \t%f\n", t_omp);

    printf("\nSpeedup t_omp : %f\n", t_seq/t_omp);

    printf("\nEnd of test\n");

    free(image);
    free(image_edges);

    return 0;
}

void reduce_noise  (unsigned char *image, const int width, const int height){

    // walk through picture and average values
    // look at all neighbouring pixels
    // omit edge pixels
    // (first and last row, as well as first and last column)
    // each neighbour is multiplied with a different weight
    // and the result is averaged
    for (int i = 1; i < height-1; i ++){
        for (int j = 1; j < width-1; j ++){
            int sum = 0;
            // neighbours from row above
            sum += 1* image[(i-1)*width + (j-1)] + 2* image[(i-1)*width + (j)] + 1* image[(i-1)*width + (j+1)];
            // same row
            sum += 2* image[(i  )*width + (j-1)] + 4* image[(i  )*width + (j)] + 2* image[(i  )*width + (j+1)];
            // row below
            sum += 1* image[(i+1)*width + (j-1)] + 2* image[(i+1)*width + (j)] + 1* image[(i+1)*width + (j+1)];

            // average pixels
            // divide by 16 (= shift right by 4)
            sum = sum >> 4;
            // clip value to max of 255
            sum = (sum > 0xFF)? 0xFF : sum;

            // store as new pixel value
            image[i*width + j]=sum;
        }
    }
}

double edge_detection_seq(unsigned char *image, unsigned char *image_edges, const int width, int height){

    double begin, end;

    // we apply a horizontal and a vertical filter
    // afterwards, we combine the results

    begin = omp_get_wtime();

    // walk through picture and apply filter
    // again, we skip the edge pixels
    // let's initialize the result array so edge pixels will remain black
    for (int k = 0; k < width*height; k++) image_edges[k] = 0x00;

    // walk through image and look at every pixel
    for (int i = 1; i < height-1; i ++){
        for (int j = 1; j < width-1; j ++){
            // calculate horizontal filter
            // filter looks at neighbours right and left of pix
            // and multiplies them with following coefficients, then sums it all up:
            // | -1 |  0  |  1 |
            // | -2 | pix |  2 |
            // | -1 |  0  |  1 |

            int hor_sum = 0;
            // row above
            hor_sum += -1* image[(i-1)*width + (j-1)] + 1* image[(i-1)*width + (j+1)];
            // same row
            hor_sum += -2* image[(i  )*width + (j-1)] + 2* image[(i  )*width + (j+1)];
            // row below
            hor_sum += -1* image[(i+1)*width + (j-1)] + 1* image[(i+1)*width + (j+1)];

            // saturate value
            hor_sum = abs(hor_sum);
            hor_sum = (hor_sum > 0xFF)? 0xFF: hor_sum;


            // rinse and repeat for vertical filter
            // filter looks at neighbours above and below of pix
            // and multiplies with following coefficients, then sums it all up:
            // | -1 | -2  | -1 |
            // |  0 | pix |  0 |
            // |  1 |  2  |  1 |
            int ver_sum = 0;
            // row above
            ver_sum += -1* image[(i-1)*width + (j-1)] + -2* image[(i-1)*width + (j)] + -1* image[(i-1)*width + (j+1)];
            // row below
            ver_sum +=  1* image[(i+1)*width + (j-1)] +  2* image[(i+1)*width + (j)] +  1* image[(i+1)*width + (j+1)];

            // saturate value
            ver_sum = abs(ver_sum);
            ver_sum = (ver_sum > 0xFF)? 0xFF: ver_sum;

            // now, calculate final results
            // image_edges = sqrt(hor_sum² + ver_sum²);
            int tmp = hor_sum * hor_sum + ver_sum * ver_sum;
            tmp = sqrt(tmp);
            // saturate and store
            image_edges[i*width + j] = (tmp > 0xFF)? 0xFF : tmp;
        }
    }

    end = omp_get_wtime();

    return (double)(end - begin);

}

double edge_detection_omp(unsigned char *image, unsigned char *image_edges, const int width, int height) {
    double begin, end;
    begin = omp_get_wtime();

    //omp_set_num_threads(NUM_THREADS);
    #pragma omp parallel
    {
        //printf("%d\n", omp_get_num_threads());
        #pragma omp for schedule(static)
        for(int k = 0; k < width * height; k++) image_edges[k] = 0x00;

        #pragma omp for schedule(static)
        for (int i = 1; i < height - 1; i++) {
            for (int j = 1; j < width - 1; j++) {
                int hor_sum = 0;
                hor_sum += -1 * image[(i - 1) * width + (j - 1)] + 1 * image[(i - 1) * width + (j + 1)];
                hor_sum += -2 * image[(i) * width + (j - 1)] + 2 * image[(i) * width + (j + 1)];
                hor_sum += -1 * image[(i + 1) * width + (j - 1)] + 1 * image[(i + 1) * width + (j + 1)];

                hor_sum = abs(hor_sum);
                hor_sum = (hor_sum > 0xFF) ? 0xFF : hor_sum;

                int ver_sum = 0;
                ver_sum += -1 * image[(i - 1) * width + (j - 1)] + -2 * image[(i - 1) * width + (j)] +
                           -1 * image[(i - 1) * width + (j + 1)];
                ver_sum += 1 * image[(i + 1) * width + (j - 1)] + 2 * image[(i + 1) * width + (j)] +
                           1 * image[(i + 1) * width + (j + 1)];

                ver_sum = abs(ver_sum);
                ver_sum = (ver_sum > 0xFF) ? 0xFF : ver_sum;
                int tmp = hor_sum * hor_sum + ver_sum * ver_sum;
                tmp = sqrt(tmp);
                //#pragma omp atomic
                image_edges[i * width + j] = (tmp > 0xFF) ? 0xFF : tmp;
            }
        }
    }
    end = omp_get_wtime();
    return (end-begin);
}


unsigned char * readPGM(const char * path, int * width, int * height){

    printf("Reading file: %s \n", path);
    FILE *imgFile = fopen(path, "rb");
    if (imgFile == NULL){
        printf("Unable to open file!\n");
        return NULL;
    }

    char lineBuff[255];

    // read header
    // check if picture is grayscale or color
    int factor = 1;
    fgets(lineBuff, 255, imgFile);
    if (strcmp(lineBuff,"P6")){
        // colored picture, 3 values per pixel (RGB)
        factor = 3;
    }
    else if (strcmp(lineBuff, "P5")){
        factor = 1;
    }
    else {
        printf("Wrong file format! Expected P5 or P6, got %s\n", lineBuff);
        return NULL;
    }

    fgets(lineBuff, 255, imgFile);
    // remove comments from file, if needed
    while (lineBuff[0]== '#'){
        fgets(lineBuff, 255, imgFile);
    }
    sscanf(lineBuff, "%d %d", width, height);
    printf("width: %d, height: %d\n", *width, *height);

    fgets(lineBuff, 255, imgFile);
    if (!strcmp(lineBuff, "255")){
        printf("Wrong file format! Expected 8-b pixel value (max. 255), got %s\n", lineBuff);
        return NULL;
    }

    // now, we can allocate memory and assign it to pointer passed by argument
    int picSize = (*width) * (*height) * factor;
    unsigned char * img = (unsigned char*) malloc(picSize * sizeof(char));

    if (img == NULL){
        printf("Memory Allocation failed!\n");
        return NULL;
    }

    for (int i = 0; i < picSize; i++){
        img[i] = fgetc(imgFile);
    }

    fclose(imgFile);

    return img;
}

int writePGM (const char * path, unsigned char * image, int width, int height){

    printf("Writing results to %s\n", path);

    FILE *imgFile = fopen(path, "wb");

    if (imgFile == NULL){
        printf("Unable to open file!\n");
        return 1;
    }

    fputs("P5\n", imgFile);

    fputs("#file generated by apo code \n", imgFile);
    // create string for "width height" line
    fprintf(imgFile, "%d %d\n255\n", width, height);

    for (int i = 0; i < width * height; ++i){
        fputc(image[i], imgFile);
    }

    return fclose(imgFile);

}
