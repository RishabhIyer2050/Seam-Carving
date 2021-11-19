#include <stdio.h>
#include <math.h>
#include "seamcarving.h"

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