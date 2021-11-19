#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <stdint.h>

struct rgb_img{
    uint8_t *raster;
    size_t height;
    size_t width;
};

void create_img(struct rgb_img **im, size_t height, size_t width){
    *im = (struct rgb_img *)malloc(sizeof(struct rgb_img));
    (*im)->height = height;
    (*im)->width = width;
    (*im)->raster = (uint8_t *)malloc(3 * height * width);
}


int read_2bytes(FILE *fp){
    uint8_t bytes[2];
    fread(bytes, sizeof(uint8_t), 1, fp);
    fread(bytes+1, sizeof(uint8_t), 1, fp);
    return (  ((int)bytes[0]) << 8)  + (int)bytes[1];
}

void write_2bytes(FILE *fp, int num){
    uint8_t bytes[2];
    bytes[0] = (uint8_t)((num & 0XFFFF) >> 8);
    bytes[1] = (uint8_t)(num & 0XFF);
    fwrite(bytes, 1, 1, fp);
    fwrite(bytes+1, 1, 1, fp);
}

void read_in_img(struct rgb_img **im, char *filename){
    FILE *fp = fopen(filename, "rb");
    size_t height = read_2bytes(fp);
    size_t width = read_2bytes(fp);
    create_img(im, height, width);
    fread((*im)->raster, 1, 3*width*height, fp);
    fclose(fp);
}

void write_img(struct rgb_img *im, char *filename){
    FILE *fp = fopen(filename, "wb");
    write_2bytes(fp, im->height);
    write_2bytes(fp, im->width);
    fwrite(im->raster, 1, im->height * im->width * 3, fp);
    fclose(fp);
}

uint8_t get_pixel(struct rgb_img *im, int y, int x, int col){
    return im->raster[3 * (y*(im->width) + x) + col];
}

void set_pixel(struct rgb_img *im, int y, int x, int r, int g, int b){
    im->raster[3 * (y*(im->width) + x) + 0] = r;
    im->raster[3 * (y*(im->width) + x) + 1] = g;
    im->raster[3 * (y*(im->width) + x) + 2] = b;
}

void destroy_image(struct rgb_img *im)
{
    free(im->raster);
    free(im);
}


void print_grad(struct rgb_img *grad){
    int height = grad->height;
    int width = grad->width;
    for(int i = 0; i < height; i++){
        for(int j = 0; j < width; j++){
            printf("%d\t", get_pixel(grad, i, j, 0));
        }
    printf("\n");    
    }
}

void print_img(struct rgb_img *grad){
    int height = grad->height;
    int width = grad->width;
    for(int i = 0; i < height; i++){
        for(int j = 0; j < width; j++){
            printf("(%d\t", get_pixel(grad, i, j, 0));
            printf("%d\t", get_pixel(grad, i, j, 1));
            printf("%d)\t", get_pixel(grad, i, j, 2));
        }
    printf("\n");    
    }
}

// ----------------------------------------------------------------------------------------------- //

void calc_energy(struct rgb_img *im, struct rgb_img **grad){
    
    double valx = 0;
    double valy = 0;

    int height = im->height;
    int width = im->width;

    *grad = (struct rgb_img *)malloc(sizeof(struct rgb_img));
    (*grad)->height = height;
    (*grad)->width = width;
    (*grad)->raster = (uint8_t *)malloc(3 * height * width);

    for(int i=0; i<height; i++){
        for(int j=0; j<width; j++){
            for(int rgb=0; rgb<3; rgb++){

                double valx_p = get_pixel(im, i, j==0 ? width-1 : j-1, rgb);
                double valx_n = get_pixel(im, i, j==(width-1) ? 0 : j+1, rgb);
                double valy_p = get_pixel(im, i==0 ? height-1 : i-1, j, rgb);
                double valy_n = get_pixel(im, i==(height-1) ? 0 : i+1, j, rgb);

                valx = valx + pow((valx_n-valx_p), 2);
                valy = valy + pow((valy_n-valy_p), 2);
            }
            int fval = sqrt(valx + valy)/10;
            set_pixel(*grad, i, j, fval, fval, fval);
            valx = 0;
            valy = 0;
        }
    }
}

void dynamic_seam(struct rgb_img *grad, double **best_arr){

    int height = grad->height;
    int width = grad->width;

    (*best_arr) = (double *)malloc(sizeof(double)*height*width);

    double min = INFINITY;
    
    for(int i=0; i<height; i++){
        for(int j=0; j<width; j++){
            if(i==0){
                (*best_arr)[i*width+j] = (double)get_pixel(grad, i, j, 0);
                //printf("%lf\n", (*best_arr)[i*width+j]);
            }
            else{
                double c1 = j==0 ? INFINITY : (*best_arr)[(i-1)*width+(j-1)];
                double c2 = (*best_arr)[(i-1)*width+j];
                double c3 = j==(width-1) ? INFINITY : (*best_arr)[(i-1)*width+(j+1)];

                if(c1<c2){
                    min = c1;
                }
                else{
                    min = c2;
                }

                if(c3<min){
                    min = c3;
                }

                (*best_arr)[i*width+j] = min + get_pixel(grad, i, j, 0);
                //printf("%lf\n", (*best_arr)[i*width+j]);
            }
        }
    }
}

void recover_path(double *best, int height, int width, int **path){

    int g_min = INFINITY;
    int cur = 0;

    *path = (int *)malloc(sizeof(int)*height);
    memset((*path), 0, height);

    for(int i=(height-1); i>=0; i--){
            if(i==(height-1)){
                for(int j=0; j<width; j++){
                    cur = best[i*width+j];
                    if(cur<g_min){
                        g_min = cur;
                        (*path)[i] = j;
                    }
                }        
            }
            else{
                int min_ind = 0;
                double c1 = (*path)[i+1]==0 ? INFINITY : (best)[(i)*width+((*path)[i+1]-1)];
                double c2 = (best)[(i)*width+(*path)[i+1]];
                double c3 = (*path)[i+1]==(width-1) ? INFINITY : (best)[(i)*width+((*path)[i+1]+1)];

                if(c1<=c2){
                    min_ind = ((*path)[i+1]-1);
                    g_min = c1;
                }
                else{
                    min_ind = (*path)[i+1];
                    g_min = c2;
                }
                if(c3<g_min){
                    min_ind = ((*path)[i+1]+1);
                    g_min = c3;
                }
                (*path)[i] = min_ind;
            }
        //printf("%ld\n", (*path)[i]);
    }
}


void remove_seam(struct rgb_img *src, struct rgb_img **dest, int *path){
    
    int height = src->height;
    int width = src->width;

    *dest = (struct rgb_img *)malloc(sizeof(struct rgb_img));
    (*dest)->height = height;
    (*dest)->width = width-1;
    (*dest)->raster = (uint8_t *)malloc(3 * height * (width-1));

    for(int i=0; i<height; i++){
        for(int j=0; j<width; j++){
            if(j<path[i]){
                int r = get_pixel(src, i, j, 0);
                int g = get_pixel(src, i, j, 1);
                int b = get_pixel(src, i, j, 2);
                set_pixel(*dest, i, j, r, g, b);
                //printf("\n");
                //print_img(*dest);
            }
            else if(j>path[i]){
                int r = get_pixel(src, i, j, 0);
                int g = get_pixel(src, i, j, 1);
                int b = get_pixel(src, i, j, 2);
                set_pixel(*dest, i, j-1, r, g, b);
                //printf("\n");
                //print_img(*dest);
            }
        }
    }
}

void main(void){
    struct rgb_img *im;
    struct rgb_img *cur_im;
    struct rgb_img *grad;
    double *best;
    int *path;

    // Enter Image path (.bin file)
    read_in_img(&im, "HJoceanSmall.bin");
    
    for(int i = 0; i < 200; i++){
        printf("i = %d\n", i);
        calc_energy(im,  &grad);
        dynamic_seam(grad, &best);
        recover_path(best, grad->height, grad->width, &path);
        remove_seam(im, &cur_im, path);
        
    if(i==99){
        char filename[200];
        sprintf(filename, "img%d.bin", 200);
        write_img(cur_im, filename);}


        destroy_image(im);
        destroy_image(grad);
        free(best);
        free(path);
        im = cur_im;
    }
    
    destroy_image(im);
}
