/*  Store Data at the end of simulation */

#define S_FUNCTION_NAME savedata
#define S_FUNCTION_LEVEL 2

#include "simstruc.h"
#include "stdio.h"     /* for file handling */

/* mdlCheckParameters, check parameters, this routine is called later from mdlInitializeSizes */
#define MDL_CHECK_PARAMETERS
static void mdlCheckParameters(SimStruct *S)
{
    /* Basic check : All parameters must be real positive vectors                             */
    real_T *pr;                            
    int_T  i, el, nEls;

    for (i = 0; i < 3; i++) {
        if (mxIsEmpty(    ssGetSFcnParam(S,i)) || mxIsSparse(   ssGetSFcnParam(S,i)) ||
            mxIsComplex(  ssGetSFcnParam(S,i)) || !mxIsNumeric( ssGetSFcnParam(S,i))  )
                  { ssSetErrorStatus(S,"Parameters must be real finite vectors"); return; } 
        pr   = mxGetPr(ssGetSFcnParam(S,i));
        nEls = mxGetNumberOfElements(ssGetSFcnParam(S,i));
        for (el = 0; el < nEls; el++) {
            if (!mxIsFinite(pr[el])) 
                  { ssSetErrorStatus(S,"Parameters must be real finite vectors"); return; }
        }
    }

    /* Check number of elements in number of channels parameter */
    if ( mxGetNumberOfElements(ssGetSFcnParam(S,0)) != 1 )
    { ssSetErrorStatus(S,"The parameter must be a scalar"); return; }

    /* get the basic parameters and check them */
    pr = mxGetPr(ssGetSFcnParam(S,0));
    if ( (pr[0] < 1) )
    { ssSetErrorStatus(S,"The number of channels must be greater than zero"); return; }


    /* Check number of elements in [T_start T_end] parameter */
    if ( mxGetNumberOfElements(ssGetSFcnParam(S,1)) != 2 )
    { ssSetErrorStatus(S,"[T_start T_end] must be a two element vector"); return; }

    /* get the basic parameters and check them */
    pr = mxGetPr(ssGetSFcnParam(S,1));
    if ( (pr[0] < 0) | (pr[1] < 0))
    { ssSetErrorStatus(S,"[T_start T_end] must be non negative"); return; }
    if (pr[0] >= pr[1])
    { ssSetErrorStatus(S,"T_end must be greater than T_start"); return; }

    /* Check number of elements in sampling time parameter */
    if ( mxGetNumberOfElements(ssGetSFcnParam(S,2)) != 1 )
    { ssSetErrorStatus(S,"The parameter must be a scalar"); return; }

    /* get the sampling time and check it */
    pr = mxGetPr(ssGetSFcnParam(S,2));
    if ( (pr[0] <= 0) )
    { ssSetErrorStatus(S,"The sampling time must be greater than zero"); return; }

	/* get the input type and check it */
    pr = mxGetPr(ssGetSFcnParam(S,5));
    if ( ((int)(pr[0]-0.7) < 0) || ((int)(pr[0]-0.7) > 8) )
    { ssSetErrorStatus(S,"The output type must be an integer between 0 and 8"); return; }

}


/* mdlInitializeSizes - initialize the sizes array ********************************************/
static void mdlInitializeSizes(SimStruct *S) {

    real_T *NumChannels, *SampTime, *SimTime; 

	int Ti =(int) (*mxGetPr(ssGetSFcnParam(S,5))-0.7);    /* get the input type                   */

    ssSetNumSFcnParams(S,6);                          /* number of expected parameters        */

    /* Check the number of parameters and then calls mdlCheckParameters to see if they are ok */
    if (ssGetNumSFcnParams(S) == ssGetSFcnParamsCount(S))
    { 
		mdlCheckParameters(S); 
		if (ssGetErrorStatus(S) != NULL) return; 
	} 
	else return;

    ssSetNumContStates(S,0);                          /* number of continuous states          */
    ssSetNumDiscStates(S,0);                          /* number of discrete states            */

    NumChannels = mxGetPr(ssGetSFcnParam(S,0));		  /* Access the number of channels parameter */
    SimTime		= mxGetPr(ssGetSFcnParam(S,1));		  /* Access the start and end time of the simulation */	
	SampTime	= mxGetPr(ssGetSFcnParam(S,2));		  /* Access the sampling time parameter   */
	

    if (!ssSetNumInputPorts(S,1)) return;             /* number of input ports                */
    ssSetInputPortWidth(S,0,*NumChannels);			  /* first input port width               */
    ssSetInputPortDirectFeedThrough(S,0,1);           /* first port direct feedthrough flag   */

	/* set the input data type, basically equivalent to ssSetInputPortDataType(S,0,Ti);       */
	switch (Ti) {
	case 0:
		ssSetInputPortDataType(S,0,SS_DOUBLE);
		break;
	case 1:
		ssSetInputPortDataType(S,0,SS_SINGLE);
		break;
	case 2:
		ssSetInputPortDataType(S,0,SS_INT8);
		break;
	case 3:
		ssSetInputPortDataType(S,0,SS_UINT8);
		break;
	case 4:
		ssSetInputPortDataType(S,0,SS_INT16);
		break;
	case 5:
		ssSetInputPortDataType(S,0,SS_UINT16);
		break;
	case 6:
		ssSetInputPortDataType(S,0,SS_INT32);
		break;
	case 7:
		ssSetInputPortDataType(S,0,SS_UINT32);
		break;
	case 8:
		ssSetInputPortDataType(S,0,SS_BOOLEAN);
		break;
	default:
		ssSetErrorStatus(S,"Error in mdlInitializeSizes : input port type unrecognized"); 
		return;
	}

    if (!ssSetNumOutputPorts(S,0)) return;            /* number of output ports               */
    
    ssSetNumSampleTimes(S,0);                         /* number of sample times               */

	ssSetNumRWork(S,0);								  /* number work vector elements		  */
    ssSetNumIWork(S,0);                               /* number int_T work vector elements    */
    ssSetNumPWork(S,1);                               /* number ptr work vector elements      */
    ssSetNumModes(S,0);                               /* number mode work vector elements     */
    ssSetNumNonsampledZCs(S,0);                       /* number of nonsampled zero crossing   */
}

/* mdlInitializeSampleTimes - initialize the sample times array *******************************/
static void mdlInitializeSampleTimes(SimStruct *S) {

	real_T *pr = mxGetPr(ssGetSFcnParam(S,2));

    ssSetSampleTime(S, 0, *pr);
    ssSetOffsetTime(S, 0, 0);
}

/* mdlStart - initialize hardware *************************************************************/
#define MDL_START
static void mdlStart(SimStruct *S) {

    real_T *NumChannels, *SampTime, *SimTime; 
	int_T i, Si, NumberOfElements;
	unsigned char *Buffer;

	/* input port data type */
	DTypeId Ti=ssGetInputPortDataType(S,0);

    /* retrieve pointer to pointers work vector */
	void **PWork = ssGetPWork(S);

	/* calculate input datatype size in bytes */
	switch (Ti) {
	case SS_DOUBLE:
		Si=sizeof(real_T);
		break;
	case SS_SINGLE:
		Si=sizeof(float);
		break;
	case SS_INT8:
		Si=sizeof(int8_T);
		break;
	case SS_UINT8:
		Si=sizeof(uint8_T);
		break;
	case SS_INT16:
		Si=sizeof(int16_T);
		break;
	case SS_UINT16:
		Si=sizeof(uint16_T);
		break;
	case SS_INT32:
		Si=sizeof(int32_T);
		break;
	case SS_UINT32:
		Si=sizeof(uint32_T);
		break;
	case SS_BOOLEAN:
		Si=sizeof(boolean_T);
		break;
	default:
		ssSetErrorStatus(S,"Error in mdlStart : input port type unrecognized"); 
		return;
	}

    NumChannels = mxGetPr(ssGetSFcnParam(S,0));		  /* Get the number of channels */
    SimTime		= mxGetPr(ssGetSFcnParam(S,1));		  /* Get the start and end time of the simulation */	
	SampTime	= mxGetPr(ssGetSFcnParam(S,2));		  /* Get the sampling time */

	NumberOfElements = ((int_T) ((SimTime[1]-SimTime[0])/(*SampTime)+1.5))*((int_T) *NumChannels);

	/* allocate memory for the buffer, in bytes */ 
    Buffer = malloc(Si*NumberOfElements);

	/* store pointers in PWork so they can be accessed later */
	PWork[0] = (void*) Buffer;

	/* check if memory allocation was ok */
	if (PWork[0]==NULL) 
		{ ssSetErrorStatus(S,"Error in mdlStart : could not allocate memory"); return; }

	/* zero out memory before starting */
	for(i=0;i<Si*NumberOfElements;i++)
		Buffer[i]=0;
	
}

/* mdlOutputs - compute the outputs ***********************************************************/
static void mdlOutputs(SimStruct *S, int_T tid) {

	int_T i=0,j=0,n;
	real_T *NumChannels, *SampTime, *SimTime;
	real_T ctime = ssGetT(S);
	real_T *Buffer;

    /* input ports */
	InputPtrsType u = ssGetInputPortSignalPtrs(S,0);
	real_T **ud;
	float **us;
	int8_T **u8;
	uint8_T **uu8;
	int16_T **u16;
	uint16_T **uu16;
	int32_T **u32;
	uint32_T **uu32;
	boolean_T **ub;

	/* pointers to the same buffer */
	real_T *Bd;
	float *Bs;
	int8_T *B8;
	uint8_T *Bu8;
	int16_T *B16;
	uint16_T *Bu16;
	int32_T *B32;
	uint32_T *Bu32;
	boolean_T *Bb;

	/* input port data type */
	DTypeId Ti=ssGetInputPortDataType(S,0);

    /* retrieve pointer to pointers work vector */
	void **PWork = ssGetPWork(S);

	NumChannels = mxGetPr(ssGetSFcnParam(S,0));		  /* Get the number of channels */
	SimTime	= mxGetPr(ssGetSFcnParam(S,1));			  /* Get the simulation start & stop times  */
	SampTime	= mxGetPr(ssGetSFcnParam(S,2));		  /* Get the sampling time */

	n = (int_T) *NumChannels;

	if ( (ctime >= SimTime[0]) && (ctime <= SimTime[1]) ) {
		i = (int_T) ((ctime-SimTime[0])/(*SampTime)+0.5);

		switch (Ti) {
			case SS_DOUBLE:
				Bd = (real_T*) PWork[0];
				ud = (real_T**) u;
				for(j=0;j<n;j++)
					Bd[n*i+j]=(*ud[j]);
				break;
			case SS_SINGLE:
				Bs = (float*) PWork[0];
				us = (float**) u;
				for(j=0;j<n;j++)
					Bs[n*i+j]=(*us[j]);
				break;
			case SS_INT8:
				B8 = (int8_T*) PWork[0];
				u8 = (int8_T**) u;
				for(j=0;j<n;j++)
					B8[n*i+j]=(*u8[j]);
				break;
			case SS_UINT8:
				Bu8 = (uint8_T*) PWork[0];
				uu8 = (uint8_T**) u;
				for(j=0;j<n;j++)
					Bu8[n*i+j]=(*uu8[j]);
				break;
			case SS_INT16:
				B16 = (int16_T*) PWork[0];
				u16 = (int16_T**) u;
				for(j=0;j<n;j++)
					B16[n*i+j]=(*u16[j]);
				break;
			case SS_UINT16:
				Bu16 = (uint16_T*) PWork[0];
				uu16 = (uint16_T**) u;
				for(j=0;j<n;j++)
					Bu16[n*i+j]=(*uu16[j]);
				break;
			case SS_INT32:
				B32 = (int32_T*) PWork[0];
				u32 = (int32_T**) u;
				for(j=0;j<n;j++)
					B32[n*i+j]=(*u32[j]);
				break;
			case SS_UINT32:
				Bu32 = (uint32_T*) PWork[0];
				uu32 = (uint32_T**) u;
				for(j=0;j<n;j++)
					Bu32[n*i+j]=(*uu32[j]);
				break;
			case SS_BOOLEAN:
				Bb = (boolean_T*) PWork[0];
				ub = (boolean_T**) u;
				for(j=0;j<n;j++)
					Bb[n*i+j]=(*ub[j]);
				break;
			default:
				ssSetErrorStatus(S,"Error in mdlOutput : input port type unrecognized"); 
				return;
		}
	
	}
	
}

/* mdlTerminate - called when the simulation is terminated */
static void mdlTerminate(SimStruct *S) {

    real_T *NumChannels, *SampTime, *SimTime; 
	int_T i, j, NumberOfDataPoints, n;
	FILE *data_file; /* pointer to file */
	unsigned char FileName[16], FpOutStr[16];

    /* retrieve pointer to pointers work vector */
	void **PWork = ssGetPWork(S);

	/* input port data type */
	DTypeId Ti=ssGetInputPortDataType(S,0);

	/* pointers to the same buffer */
	real_T *Bd;
	float *Bs;
	int8_T *B8;
	uint8_T *Bu8;
	int16_T *B16;
	uint16_T *Bu16;
	int32_T *B32;
	uint32_T *Bu32;
	boolean_T *Bb;

	/* get the file name */
    mxGetString(ssGetSFcnParam(S,3),FileName,16);
    mxGetString(ssGetSFcnParam(S,4),FpOutStr,16);

    NumChannels = mxGetPr(ssGetSFcnParam(S,0));		  /* Access the number of channels parameter */
    SimTime		= mxGetPr(ssGetSFcnParam(S,1));		  /* Access the start and end time of the simulation */	
	SampTime	= mxGetPr(ssGetSFcnParam(S,2));		  /* Access the sampling time parameter   */
	NumberOfDataPoints = (int_T) ((SimTime[1]-SimTime[0])/(*SampTime)+ 1.5);
	
	n = (int_T) *NumChannels;

	/* open data file */
    data_file = fopen(FileName,"w"); 

	/* check out if it has actually been opened */
	if (data_file==NULL) { ssSetErrorStatus(S,"Could not open the data file"); return; }

	switch (Ti) {
	case SS_DOUBLE:

		/* retrieve Buffer Pointer */
		Bd = (real_T*) PWork[0];

		/* Write the data into the file */
		for (i=0; i < NumberOfDataPoints; i++) {
			for (j=0; j < n; j++) {
				fprintf(data_file,FpOutStr,Bd[n*i+j]);
			}
			fprintf(data_file,"\n");
		}
 
		/* close file */
		fclose(data_file); 

		/* free memory */
		free(Bd);

		break;

	case SS_SINGLE:

		/* retrieve Buffer Pointer */
		Bs = (float*) PWork[0];

		/* Write the data into the file */
		for (i=0; i < NumberOfDataPoints; i++) {
			for (j=0; j < n; j++) {
				fprintf(data_file,FpOutStr,Bs[n*i+j]);
			}
			fprintf(data_file,"\n");
		}
 
		/* close file */
		fclose(data_file); 

		/* free memory */
		free(Bs);

		break;

	case SS_INT8:

		/* retrieve Buffer Pointer */
		B8 = (int8_T*) PWork[0];

		/* Write the data into the file */
		for (i=0; i < NumberOfDataPoints; i++) {
			for (j=0; j < n; j++) {
				fprintf(data_file,FpOutStr,B8[n*i+j]);
			}
			fprintf(data_file,"\n");
		}
 
		/* close file */
		fclose(data_file); 

		/* free memory */
		free(B8);

		break;

	case SS_UINT8:

		/* retrieve Buffer Pointer */
		Bu8 = (uint8_T*) PWork[0];

		/* Write the data into the file */
		for (i=0; i < NumberOfDataPoints; i++) {
			for (j=0; j < n; j++) {
				fprintf(data_file,FpOutStr,Bu8[n*i+j]);
			}
			fprintf(data_file,"\n");
		}
 
		/* close file */
		fclose(data_file); 

		/* free memory */
		free(Bu8);

		break;

	case SS_INT16:

		/* retrieve Buffer Pointer */
		B16 = (int16_T*) PWork[0];

		/* Write the data into the file */
		for (i=0; i < NumberOfDataPoints; i++) {
			for (j=0; j < n; j++) {
				fprintf(data_file,FpOutStr,B16[n*i+j]);
			}
			fprintf(data_file,"\n");
		}
 
		/* close file */
		fclose(data_file); 

		/* free memory */
		free(B16);

		break;

	case SS_UINT16:

		/* retrieve Buffer Pointer */
		Bu16 = (uint16_T*) PWork[0];

		/* Write the data into the file */
		for (i=0; i < NumberOfDataPoints; i++) {
			for (j=0; j < n; j++) {
				fprintf(data_file,FpOutStr,Bu16[n*i+j]);
			}
			fprintf(data_file,"\n");
		}
 
		/* close file */
		fclose(data_file); 

		/* free memory */
		free(Bu16);

		break;

	case SS_INT32:

		/* retrieve Buffer Pointer */
		B32 = (int32_T*) PWork[0];

		/* Write the data into the file */
		for (i=0; i < NumberOfDataPoints; i++) {
			for (j=0; j < n; j++) {
				fprintf(data_file,FpOutStr,B32[n*i+j]);
			}
			fprintf(data_file,"\n");
		}
 
		/* close file */
		fclose(data_file); 

		/* free memory */
		free(B32);

		break;

	case SS_UINT32:

		/* retrieve Buffer Pointer */
		Bu32 = (uint32_T*) PWork[0];

		/* Write the data into the file */
		for (i=0; i < NumberOfDataPoints; i++) {
			for (j=0; j < n; j++) {
				fprintf(data_file,FpOutStr,Bu32[n*i+j]);
			}
			fprintf(data_file,"\n");
		}
 
		/* close file */
		fclose(data_file); 

		/* free memory */
		free(Bu32);

		break;

	case SS_BOOLEAN:

		/* retrieve Buffer Pointer */
		Bb = (boolean_T*) PWork[0];

		/* Write the data into the file */
		for (i=0; i < NumberOfDataPoints; i++) {
			for (j=0; j < n; j++) {
				fprintf(data_file,FpOutStr,Bb[n*i+j]);
			}
			fprintf(data_file,"\n");
		}
 
		/* close file */
		fclose(data_file); 

		/* free memory */
		free(Bb);

		break;

	default:
		ssSetErrorStatus(S,"Error in mdlTerminate : input port type unrecognized"); 
	return;

	}

}

/* Trailer information to set everything up for simulink usage *******************************/
#ifdef  MATLAB_MEX_FILE                      /* Is this file being compiled as a MEX-file?   */
#include "simulink.c"                        /* MEX-file interface mechanism                 */
#else
#include "cg_sfun.h"                         /* Code generation registration function        */
#endif
