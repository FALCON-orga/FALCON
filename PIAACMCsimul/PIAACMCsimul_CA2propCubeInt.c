/**
 * @file    PIAACMCsimul_CA2propCubeInt.c
 * @brief   PIAA-type coronagraph design
 *
 * Can design both APLCMC and PIAACMC coronagraphs
 *
 * @author  O. Guyon
 * @date    21 nov 2017
 *
 *
 * @bug No known bugs.
 *
 */



#include <stdlib.h>
#include <stdio.h>

#include "CommandLineInterface/CLIcore.h"
#include "COREMOD_memory/COREMOD_memory.h"

#include "OptSystProp/OptSystProp.h"



extern OPTSYST *optsyst;





/**
 * @brief Propagate complex amplitude image into intensity map cube
 *
 *
 */

long PIAACMCsimul_CA2propCubeInt(
    const char *IDamp_name,
    const char *IDpha_name,
    float zmin,
    float zmax,
    long NBz,
    const char *IDout_name
)
{
    DEBUG_TRACE_FSTART();

    imageID IDout;
    long nblambda;
    float *zarray;
    float zprop;

    uint32_t xsize;
    uint32_t ysize;

#ifdef PIAASIMUL_LOGFUNC0
    PIAACMCsimul_logFunctionCall("PIAACMCsimul.fcall.log", __FUNCTION__, __LINE__,
                                 "");
#endif


    printf("PIAACMCsimul_CA2propCubeInt   : %s %s %f %f %ld %s\n", IDamp_name,
           IDpha_name, zmin, zmax, NBz, IDout_name);
    fflush(stdout);

    //  list_image_ID();
    {
        imageID IDa = image_ID(IDamp_name);
        xsize = data.image[IDa].md[0].size[0];
        ysize = data.image[IDa].md[0].size[1];

        create_2Dimage_ID("retmpim", xsize, ysize);
        create_2Dimage_ID("imtmpim", xsize, ysize);

        if(data.image[IDa].md[0].naxis == 3)
        {
            nblambda = data.image[IDa].md[0].size[2];
        }
        else
        {
            nblambda = 1;
        }
    }
    IDout = create_3Dimage_ID(IDout_name, xsize, ysize, NBz);


    create_2Dimage_ID("tmpintg", xsize, ysize);


    // initialize zarray
    zarray = (float *) malloc(sizeof(float) * NBz);
    if(zarray == NULL) {
        PRINT_ERROR("malloc returns NULL pointer");
        abort(); // or handle error in other ways
    }

    for(long l = 0; l < NBz; l++)
    {
        zarray[l] = zmin + (zmax - zmin) * l / (NBz - 1);
    }



    for(long l = 0; l < NBz; l++)
    {
        printf("l = %ld/%ld\n", l, NBz);
        fflush(stdout);

        zprop = zarray[l];
        OptSystProp_propagateCube(optsyst, 0, IDamp_name, IDpha_name, "_tmppropamp",
                                  "_tmpproppha", zprop, 0);

        imageID IDa = image_ID("_tmppropamp");
        // imageID IDp = image_ID("_tmpproppha");


        // write intensity
        for(long k = 0; k < nblambda; k++)
            for(long ii = 0; ii < xsize * ysize; ii++)
            {
                data.image[IDout].array.F[l * xsize * ysize + ii] += data.image[IDa].array.F[ii]
                        * data.image[IDa].array.F[k * xsize * ysize + ii];
            }



        delete_image_ID("_tmppropamp", DELETE_IMAGE_ERRMODE_WARNING);
        delete_image_ID("_tmpproppha", DELETE_IMAGE_ERRMODE_WARNING);
    }

    free(zarray);
    delete_image_ID("retmpim", DELETE_IMAGE_ERRMODE_WARNING);
    delete_image_ID("imtmpim", DELETE_IMAGE_ERRMODE_WARNING);

    DEBUG_TRACE_FEXIT();
    return IDout;
}

