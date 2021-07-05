/**
 * @file    PIAAACMCsimul_mkFocalPlaneMask.c
 * @brief   PIAA-type coronagraph design, make focal plane mask
 *
 *
 */


// System includes
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <malloc.h>


#include "CommandLineInterface/CLIcore.h"
#include "COREMOD_memory/COREMOD_memory.h"
#include "COREMOD_iofits/COREMOD_iofits.h"
#include "OpticsMaterials/OpticsMaterials.h"


#include "OptSystProp/OptSystProp.h"

#include "PIAACMCsimul.h"
#include "PIAACMCsimul_loadsavepiaacmcconf.h"


extern PIAACMCsimul_varType piaacmcsimul_var;

extern OPTSYST *optsyst;

extern OPTPIAACMCDESIGN *piaacmc;





/**
 * @brief Make complex amplitude focal plane mask
 *
 @param[in]  IDzonemap_name	zones
 @param[in]  ID_name
 @param[in]  mode       if mode = -1, make whole 1-fpm, if mode = zone, make only 1 zone with CA = (1.0, 0.0)
 @param[in]  saveMask   1 if mask saved to file system

if mode is invalid number, no focal plane mask, AND assume 1-fpm is computed

 zone numbering starts here from 1 (zone 1 = outermost ring)
*/
errno_t mkFocalPlaneMask(
    const char *IDzonemap_name,
    const char *ID_name,
    int         mode,
    int         saveMask,
    imageID    *outID
)
{
    DEBUG_TRACE_FSTART();

    uint32_t size;

    //  double fpscale; // [m/pix]
    uint64_t size2;

    uint_fast8_t CentCone = 0;
    uint_fast8_t OuterCone = 0;

    double *tarray;
    double *aarray;
    double *phaarray;
    double *cosphaarray;
    double *sinphaarray;

    long NBsubPix = 64;
    int FPMmode = 0; // 1 if using 1-fpm




#ifdef PIAASIMUL_LOGFUNC0
    PIAACMCsimul_logFunctionCall("PIAACMCsimul.fcall.log", __FUNCTION__, __LINE__,
                                 "");
#endif




    size = optsyst[0].size;
    size2 = size * size;
    int nblambda = optsyst[0].nblambda;


    imageID IDz = image_ID(IDzonemap_name);

    imageID ID;
    FUNC_CHECK_RETURN(
        create_3DCimage_ID(ID_name, size, size, nblambda, &ID)
    );

    imageID IDsag;
    FUNC_CHECK_RETURN(
        create_3Dimage_ID("fpmsag", size, size, nblambda, &IDsag)
    );

    imageID IDzone;
    FUNC_CHECK_RETURN(
        create_3Dimage_ID("fpmzone", size, size, nblambda, &IDzone)
    );


    if(piaacmc[0].NBrings > 2)
    {
        CentCone = 1;
        OuterCone = 1;
    }
    if(fabs(piaacmc[0].fpmOuterConeZ) < 1.0e-12)
    {
        OuterCone = 0;
    }



    printf("===================== Make focal plane mask  %s %s   mode=%d/%ld   [%d %d]\n",
           IDzonemap_name, ID_name, mode, piaacmc[0].focmNBzone, CentCone, OuterCone);

    if(mode == -1)
    {
        FPMmode = 1;
    }
    if(mode > piaacmc[0].focmNBzone)
    {
        FPMmode = 1;
    }

    // pixel scale [m/pix] at first wavelength in array
    double fpscale = (2.0 * piaacmc[0].beamrad / piaacmc[0].pixscale) / piaacmc[0].size /
                     piaacmc[0].fpzfactor * optsyst[0].lambdaarray[0] * piaacmc[0].Fratio;
    printf("piaacmc[0].fpmRad = %g m    fpscale[0] = %g m/pix   mode = %d\n",
           piaacmc[0].fpmRad, fpscale, mode);

    printf("Allocate memory\n");
    fflush(stdout);
    tarray = (double *) malloc(sizeof(double) * piaacmc[0].focmNBzone * nblambda);
    if(tarray == NULL) {
        PRINT_ERROR("malloc returns NULL pointer");
        abort();
    }

    aarray = (double *) malloc(sizeof(double) * piaacmc[0].focmNBzone * nblambda);
    if(aarray == NULL) {
        PRINT_ERROR("malloc returns NULL pointer");
        abort();
    }

    phaarray = (double *) malloc(sizeof(double) * piaacmc[0].focmNBzone * nblambda);
    if(phaarray == NULL) {
        PRINT_ERROR("malloc returns NULL pointer");
        abort();
    }

    cosphaarray = (double *) malloc(sizeof(double) * piaacmc[0].focmNBzone *
                                    nblambda);
    if(cosphaarray == NULL) {
        PRINT_ERROR("malloc returns NULL pointer");
        abort();
    }

    sinphaarray = (double *) malloc(sizeof(double) * piaacmc[0].focmNBzone *
                                    nblambda);
    if(sinphaarray == NULL) {
        PRINT_ERROR("malloc returns NULL pointer");
        abort();
    }

    // precompute zones phase shifts
    printf("Precompute zones phase shifts  %ld zones, %d wavelengths\n",
           piaacmc[0].focmNBzone, nblambda);
    fflush(stdout);






    for(int k = 0; k < nblambda; k++)
        for(long zi = 0; zi < piaacmc[0].focmNBzone; zi++)
        {
            //printf("lamdba %3ld  zone %4ld   ", k, zi);
            //fflush(stdout);

            //printf("  ID=%3ld  zi=%3ld -> %4ld/%4ld :\n", piaacmc[0].zonezID, zi, piaacmc[0].focmNBzone*k+zi, piaacmc[0].focmNBzone*nblambda);
            //fflush(stdout);

            // print material thickness
            tarray[piaacmc[0].focmNBzone * k + zi] =
                data.image[piaacmc[0].zonezID].array.D[zi];
            //printf("     thickness = %8.4g m\n", tarray[piaacmc[0].focmNBzone*k+zi]);
            //fflush(stdout);

            //printf("     (ID = %3ld  zi = %3ld -> pix = %4ld/%4ld)\n", piaacmc[0].zoneaID, zi, piaacmc[0].focmNBzone*k+zi, piaacmc[0].focmNBzone*nblambda);
            //fflush(stdout);
            aarray[piaacmc[0].focmNBzone * k + zi] =
                data.image[piaacmc[0].zoneaID].array.D[zi];
            //printf("     amp = %8.4f\n", aarray[piaacmc[0].focmNBzone*k+zi]);
            //fflush(stdout);

            //
            // compute phase from thickness
            // phase sign is positive for outgoing beam element ahead of main beam
            //
            phaarray[piaacmc[0].focmNBzone * k + zi] = OpticsMaterials_pha_lambda(
                        piaacmc[0].fpmmaterial_code, tarray[piaacmc[0].focmNBzone * k + zi],
                        optsyst[0].lambdaarray[k]);
            //printf("     pha = %8.4f rad\n", phaarray[piaacmc[0].focmNBzone*k+zi]);
            //fflush(stdout);

            cosphaarray[piaacmc[0].focmNBzone * k + zi] = cosf(
                        phaarray[piaacmc[0].focmNBzone * k + zi]);
            sinphaarray[piaacmc[0].focmNBzone * k + zi] = sinf(
                        phaarray[piaacmc[0].focmNBzone * k + zi]);
        }


    //printf("Entering parallel loop\n");
    //fflush(stdout);




# ifdef HAVE_LIBGOMP
    #pragma omp parallel default(shared) private(ii, jj, x, y, r, retmp, imtmp, iii, jjj, ii1, jj1, zi, t, a, fpscale, amp, pha, cospha, sinpha, ttmp, zonetmp)
    {
        #pragma omp for
# endif
        for(int k = 0; k < nblambda; k++)
        {
            fpscale = (2.0 * piaacmc[0].beamrad / piaacmc[0].pixscale) / piaacmc[0].size /
                      piaacmc[0].fpzfactor * optsyst[0].lambdaarray[k] * piaacmc[0].Fratio;
            printf("LAMBDA %3d / %3d = %10.5g m    SCALE = %10.5g m/pix   size=%4ul  rad=%g\n",
                   k, nblambda, optsyst[0].lambdaarray[k], fpscale, size, piaacmc[0].fpmRad);
            printf("Zone 0 amplitude [%ld]: %lf\n", piaacmc[0].zoneaID,
                   data.image[piaacmc[0].zoneaID].array.D[0]);
            printf("Zone 0 thickness: %g\n", data.image[piaacmc[0].zonezID].array.D[0]);
            printf("Number of zones: %ld\n", piaacmc[0].focmNBzone);
            printf("piaacmc[0].fpmRad = %g m\n", piaacmc[0].fpmRad);
            printf("piaacmc[0].fpmCentConeRad = %g m\n", piaacmc[0].fpmCentConeRad);
            printf("piaacmc[0].fpmOuterConeRad [%d] = %g m\n",  OuterCone,
                   piaacmc[0].fpmOuterConeRad);



            for(uint32_t ii = 0; ii < size; ii++)
                for(uint32_t jj = 0; jj < size; jj++)
                {
                    //printf("[ %4ld %4ld ] ", ii, jj);

                    double x = (1.0 * ii - size / 2) * fpscale; // [m]
                    double y = (1.0 * jj - size / 2) * fpscale; // [m]
                    double r = sqrt(x * x + y * y); // [m]

                    // default
                    double t = 0.0;
                    double a = 1.0;
                    //                  float pha = 0.0;
                    float cospha = 1.0;
                    float sinpha = 0.0;
                    double amp = 1.0;


                    if(OuterCone == 1)
                    {
                        if((r > 0.9 * piaacmc[0].fpmRad)
                                && (r < piaacmc[0].fpmOuterConeRad)) // outer cone
                        {
                            double t = piaacmc[0].fpmOuterConeZ * (piaacmc[0].fpmOuterConeRad - r) /
                                       (piaacmc[0].fpmOuterConeRad - piaacmc[0].fpmRad);
                            double pha = OpticsMaterials_pha_lambda(piaacmc[0].fpmmaterial_code, t,
                                                                    optsyst[0].lambdaarray[k]);
                            cospha = cosf(pha);
                            sinpha = sinf(pha);
                            a = 1.0;
                            amp = a;
                        }
                    }



                    data.image[IDzone].array.F[k * size2 + jj * size + ii] = 0;

                    if(r < 1.1 * piaacmc[0].fpmRad) //     fine sampling
                    {
                        //       printf("pix outercone ...");
                        //      fflush(stdout);

                        double retmp = 0.0;
                        double imtmp = 0.0;
                        double ttmp = 0.0;
                        double zonetmp = 0.0;
                        for(long iii = 0; iii < NBsubPix; iii++)
                        {
                            for(long jjj = 0; jjj < NBsubPix; jjj++)
                            {
                                // physical coordinates on mask
                                // x and y in [m]
                                double x = (1.0 * ii - size / 2 + 1.0 * (0.5 + iii) / NBsubPix - 0.5) * fpscale;
                                double y = (1.0 * jj - size / 2 + 1.0 * (0.5 + jjj) / NBsubPix - 0.5) * fpscale;
                                double r = sqrt(x * x + y * y); // [m]

                                long zi = 0; // default
                                cospha = 1.0;
                                sinpha = 0.0;
                                amp = 1.0;


                                if(OuterCone == 1)
                                    if((r > 0.9 * piaacmc[0].fpmRad)
                                            && (r < piaacmc[0].fpmOuterConeRad)) // outer cone
                                    {
                                        t = piaacmc[0].fpmOuterConeZ * (piaacmc[0].fpmOuterConeRad - r) /
                                            (piaacmc[0].fpmOuterConeRad - piaacmc[0].fpmRad);
                                        a = 1.0;
                                        cospha = cosphaarray[piaacmc[0].focmNBzone * k + zi - 1];
                                        sinpha = sinphaarray[piaacmc[0].focmNBzone * k + zi - 1];
                                        amp = a;
                                    }


                                long ii1 = (long) ((0.5 + 0.5 * x / piaacmc[0].fpmRad *
                                                    piaacmcsimul_var.FPMSCALEFACTOR) * piaacmc[0].fpmarraysize + 0.5);
                                long jj1 = (long) ((0.5 + 0.5 * y / piaacmc[0].fpmRad *
                                                    piaacmcsimul_var.FPMSCALEFACTOR) * piaacmc[0].fpmarraysize + 0.5);

                                if((ii1 > -1)
                                        && (ii1 < piaacmc[0].fpmarraysize)
                                        && (jj1 > -1)
                                        && (jj1 < piaacmc[0].fpmarraysize))
                                {
                                    if(CentCone == 1)
                                    {
                                        // central cone
                                        if((r < 0.99 * piaacmc[0].fpmRad) && (r < piaacmc[0].fpmCentConeRad))
                                        {
                                            t = piaacmc[0].fpmCentConeZ + (r / piaacmc[0].fpmCentConeRad) * (0.5 *
                                                    (piaacmc[0].fpmminsag + piaacmc[0].fpmmaxsag) - piaacmc[0].fpmCentConeZ);
                                            // piaacmc[0].fpmCentConeZ*(piaacmc[0].fpmCentConeRad-r)/(piaacmc[0].fpmCentConeRad); //piaacmc[0].fpmCentConeZ
                                            a = 1.0;
                                            double pha = OpticsMaterials_pha_lambda(piaacmc[0].fpmmaterial_code, t,
                                                                                    optsyst[0].lambdaarray[k]);
                                            cospha = cosf(pha);
                                            sinpha = sinf(pha);
                                            amp = a;
                                        }
                                    }

                                    // Zone number
                                    zi = (long)(data.image[IDz].array.UI16[jj1 * piaacmc[0].fpmarraysize + ii1]);
                                    if(zi - 1 > data.image[piaacmc[0].zonezID].md[0].size[0] - 1)
                                    {
                                        printf("ERROR: Zone %d does not exist (image %s has size %ld %ld)   pix %ld %ld   %ld\n",
                                               (int) zi, data.image[piaacmc[0].zonezID].md[0].name,
                                               (long) data.image[piaacmc[0].zonezID].md[0].size[0],
                                               (long) data.image[piaacmc[0].zonezID].md[0].size[1], (long) jj1, (long) jj1,
                                               piaacmc[0].fpmarraysize);
                                        exit(0);
                                    }
                                    if(zi > 0)
                                    {
                                        t = data.image[piaacmc[0].zonezID].array.D[zi - 1]; // thickness
                                        a = data.image[piaacmc[0].zoneaID].array.D[zi - 1]; // amplitude transmission
                                        cospha = cosphaarray[piaacmc[0].focmNBzone * k + zi - 1];
                                        sinpha = sinphaarray[piaacmc[0].focmNBzone * k + zi - 1];
                                        amp = a;
                                    }
                                }


                                if(FPMmode == 1)   // make 1-fpm
                                {
                                    /*                                    amp = a;
                                                                        pha = OpticsMaterials_pha_lambda(piaacmc[0].fpmmaterial_code, t, optsyst[0].lambdaarray[k]);
                                                                        cospha = cosf(pha);
                                                                        sinpha = sinf(pha);*/

//									cospha = cosphaarray[piaacmc[0].focmNBzone*k+zi-1];
//									sinpha = sinphaarray[piaacmc[0].focmNBzone*k+zi-1];

                                    retmp += 1.0 - amp * cospha;
                                    imtmp += -amp * sinpha;

                                }
                                else // impulse response from single zone
                                {
                                    if(mode == zi)
                                    {
                                        amp = 1.0;
//                                      pha = OPTICSMATERIALS_pha_lambda(piaacmc[0].fpmmaterial_code, t, optsyst[0].lambdaarray[k]);
//										cospha = cosf(pha);
//										sinpha = sinf(pha);
                                        retmp += amp;
                                        imtmp += 0.0;
                                    }
                                }

                                //bad location
                                ttmp += t;
                                zonetmp += 1.0 * zi;
                            }
                        }


                        data.image[ID].array.CF[k * size2 + jj * size + ii].re = retmp /
                                (NBsubPix * NBsubPix);
                        data.image[ID].array.CF[k * size2 + jj * size + ii].im = imtmp /
                                (NBsubPix * NBsubPix);
                        data.image[IDsag].array.F[k * size2 + jj * size + ii] = ttmp /
                                (NBsubPix * NBsubPix);
                        data.image[IDzone].array.F[k * size2 + jj * size + ii] = zonetmp /
                                (NBsubPix * NBsubPix);
                    }
                    else // coarse sampling, outside zones
                    {
                        if(FPMmode == 1)   // make 1-fpm
                        {
                            data.image[ID].array.CF[k * size2 + jj * size + ii].re =  1.0 - amp * cospha;
                            data.image[ID].array.CF[k * size2 + jj * size + ii].im = -amp * sinpha;
                        }
                        else // single zone
                        {
                            data.image[ID].array.CF[k * size2 + jj * size + ii].re = 0.0;
                            data.image[ID].array.CF[k * size2 + jj * size + ii].im = 0.0;
                        }


                        data.image[IDsag].array.F[k * size2 + jj * size + ii] = t;
                        data.image[IDzone].array.F[k * size2 + jj * size + ii] = 0.0;
                    }
                }
        }
# ifdef HAVE_LIBGOMP
    }
# endif

    printf("===================== focal plane mask  : Done\n");
    fflush(stdout);




    if(saveMask == 1)
    {
        /* save mask sag */
        //save_fits("fpmsag", "tmp_fpmsag.fits");

        PIAACMCsimul_update_fnamedescr();

        char fname[STRINGMAXLEN_FULLFILENAME];

        WRITE_FULLFILENAME(fname, "%s/fpm_sagmap2D.%s.fits.gz", piaacmcsimul_var.piaacmcconfdir,
                           piaacmcsimul_var.fnamedescr);
        FUNC_CHECK_RETURN(save_fits("fpmsag", fname));

        /* save zones */
        WRITE_FULLFILENAME(fname, "%s/fpm_zonemap2D.%s.fits.gz", piaacmcsimul_var.piaacmcconfdir,
                           piaacmcsimul_var.fnamedescr);
        FUNC_CHECK_RETURN(save_fits("fpmzone", fname));



        /* save mask transmission */
        imageID IDm;
        FUNC_CHECK_RETURN(
            create_3DCimage_ID("fpmCA", size, size, nblambda, &IDm)
        );

        for(int k = 0; k < nblambda; k++)
            for(uint32_t ii = 0; ii < size; ii++)
                for(uint32_t jj = 0; jj < size; jj++)
                {
                    data.image[IDm].array.CF[k * size2 + jj * size + ii].re = 1.0 -
                            data.image[ID].array.CF[k * size2 + jj * size + ii].re;
                    data.image[IDm].array.CF[k * size2 + jj * size + ii].im =
                        -data.image[ID].array.CF[k * size2 + jj * size + ii].im;
                    // [re,im] = 1-fpm  -> fpm = [(1.0-re), -im]
                }


        mk_amph_from_complex("fpmCA", "tfpma", "tfpmp", 0);
        FUNC_CHECK_RETURN(
            delete_image_ID("fpmCA", DELETE_IMAGE_ERRMODE_WARNING)
        );


        WRITE_FULLFILENAME(fname, "%s/fpm_CAampmap2D.%s.fits.gz", piaacmcsimul_var.piaacmcconfdir,
                           piaacmcsimul_var.fnamedescr);
        FUNC_CHECK_RETURN(
            save_fits("tfpma", fname)
        );

        WRITE_FULLFILENAME(fname, "%s/fpm_CAphamap2D.%s.fits.gz", piaacmcsimul_var.piaacmcconfdir,
                           piaacmcsimul_var.fnamedescr);
        FUNC_CHECK_RETURN(
            save_fits("tfpmp", fname)
        );

        FUNC_CHECK_RETURN(delete_image_ID("tfpma", DELETE_IMAGE_ERRMODE_WARNING));
        FUNC_CHECK_RETURN(delete_image_ID("tfpmp", DELETE_IMAGE_ERRMODE_WARNING));
    }

    FUNC_CHECK_RETURN(delete_image_ID("fpmsag", DELETE_IMAGE_ERRMODE_WARNING));

    free(tarray);
    free(aarray);
    free(phaarray);
    free(cosphaarray);
    free(sinphaarray);


    if(outID != NULL)
    {
        *outID = ID;
    }

    DEBUG_TRACE_FEXIT();
    return RETURN_SUCCESS;
}