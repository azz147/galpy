/*
  C code for the adiabatic approximation
*/
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <gsl/gsl_math.h>
#include <gsl/gsl_errno.h>
#include <gsl/gsl_roots.h>
#include <gsl/gsl_min.h>
#include <gsl/gsl_integration.h>
//Potentials
#include <galpy_potentials.h>
#include <actionAngle.h>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
/*
  Structure Declarations
*/
struct JRAdiabaticArg{
  double E;
  double Lz22delta;
  double I3U;
  double delta;
  double u0;
  double sinh2u0;
  double v0;
  double sin2v0;
  double potu0v0;
  int nargs;
  struct actionAngleArg * actionAngleArgs;
};
struct JzAdiabaticArg{
  double E;
  double Lz22delta;
  double I3V;
  double delta;
  double u0;
  double cosh2u0;
  double sinh2u0;
  double potupi2;
  int nargs;
  struct actionAngleArg * actionAngleArgs;
};
/*
  Function Declarations
*/
void actionAngleAdiabatic_actions(int,double *,double *,double *,double *,
				 double *,int,int *,double *,double,
				 double *,double *,int *);
void calcJR(int,double *,double *,double *,double *,double *,double *,
	    double,double *,double *,double *,double *,double *,int,
	    struct actionAngleArg *,int);
void calcJz(int,double *,double *,double *,double *,double *,double,
	    double *,double *,double *,double *,int,struct actionAngleArg *,
	    int);
void calcRapRperi(int,double *,double *,double *,double *,double *,double *,
		  double *,double,double *,double *,double *,double *,double *,
		  int,struct actionAngleArg *);
void calcZmax(int,double *,double *,double *,double *,double *,int,
	      struct actionAngleArg *);
double JRAdiabaticIntegrandSquared(double,void *);
double JRAdiabaticIntegrand(double,void *);
double JzAdiabaticIntegrandSquared(double,void *);
double JzAdiabaticIntegrand(double,void *);
double evaluateVerticalPotentials(double, double,int, struct actionAngleArg *);
/*
  Actual functions, inlines first
*/
inline void calcEREzL(int ndata,
		      double *R,
		      double *vR,
		      double *vT,
		      double *z,
		      double *vz,
		      double *ER,
		      double *Ez,
		      double *Lz,
		      int nargs,
		      struct actionAngleArg * actionAngleArgs){
  int ii;
  for (ii=0; ii < ndata; ii++){
    *(ER+ii)= evaluatePotentials(*(R+ii),0.,
				 nargs,actionAngleArgs)
      + 0.5 * *(vR+ii) * *(vR+ii)
      + 0.5 * *(vT+ii) * *(vT+ii);
    *(Ez+ii)= evaluateVerticalPotentials(*(R+ii),*(z+ii),
					 nargs,actionAngleArgs)     
      + 0.5 * *(vz+ii) * *(vz+ii);
    *(Lz+ii)= *(R+ii) * *(vT+ii);
  }
}
/*
  MAIN FUNCTIONS
 */
void actionAngleAdiabatic_actions(int ndata,
				  double *R,
				  double *vR,
				  double *vT,
				  double *z,
				  double *vz,
				  int npot,
				  int * pot_type,
				  double * pot_args,
				  double gamma,
				  double *jr,
				  double *jz,
				  int * err){
  int ii;
  //Set up the potentials
  struct actionAngleArg * actionAngleArgs= (struct actionAngleArg *) malloc ( npot * sizeof (struct actionAngleArg) );
  parse_actionAngleArgs(npot,actionAngleArgs,pot_type,pot_args);
  //ER, Ez, Lz
  double *ER= (double *) malloc ( ndata * sizeof(double) );
  double *Ez= (double *) malloc ( ndata * sizeof(double) );
  double *Lz= (double *) malloc ( ndata * sizeof(double) );
  calcEREzL(ndata,R,vR,vT,z,vz,ER,Ez,Lz,npot,actionAngleArgs);
  //Calculate all necessary parameters
  //for (ii=0; ii < ndata; ii++){
  //  *(coshux+ii)= cosh(*(ux+ii));
  //}
  //Calculate peri and apocenters
  double *rperi= (double *) malloc ( ndata * sizeof(double) );
  double *rap= (double *) malloc ( ndata * sizeof(double) );
  double *zmax= (double *) malloc ( ndata * sizeof(double) );
  calcZmax(ndata,jz,z,R,Ez,npot,actionAngleArgs);
  //calcJz(ndata,jz,vmin,E,Lz,I3V,delta,u0,cosh2u0,sinh2u0,potupi2,
  // npot,actionAngleArgs,10);
  //calcRapRperi(ndata,umin,umax,ux,pux,E,Lz,
  //calcJR(ndata,jr,umin,umax,E,Lz,I3U,delta,u0,sinh2u0,v0,sin2v0,potu0v0,
  //npot,actionAngleArgs,10);
}
void calcJR(int ndata,
	    double * jr,
	    double * umin,
	    double * umax,
	    double * E,
	    double * Lz,
	    double * I3U,
	    double delta,
	    double * u0,
	    double * sinh2u0,
	    double * v0,
	    double * sin2v0,
	    double * potu0v0,
	    int nargs,
	    struct actionAngleArg * actionAngleArgs,
	    int order){
  int ii;
  gsl_function JRInt;
  struct JRStaeckelArg * params= (struct JRStaeckelArg *) malloc ( sizeof (struct JRStaeckelArg) );
  params->delta= delta;
  params->nargs= nargs;
  params->actionAngleArgs= actionAngleArgs;
  //Setup integrator
  gsl_integration_glfixed_table * T= gsl_integration_glfixed_table_alloc (order);
  JRInt.function = &JRStaeckelIntegrand;
  for (ii=0; ii < ndata; ii++){
    if ( *(umin+ii) == -9999.99 || *(umax+ii) == -9999.99 ){
      *(jr+ii)= 9999.99;
      continue;
    }
    if ( (*(umax+ii) - *(umin+ii)) / *(umax+ii) < 0.000001 ){//circular
      *(jr+ii) = 0.;
      continue;
    }
    //Setup function
    params->E= *(E+ii);
    params->Lz22delta= 0.5 * *(Lz+ii) * *(Lz+ii) / delta / delta;
    params->I3U= *(I3U+ii);
    params->u0= *(u0+ii);
    params->sinh2u0= *(sinh2u0+ii);
    params->v0= *(v0+ii);
    params->sin2v0= *(sin2v0+ii);
    params->potu0v0= *(potu0v0+ii);
    JRInt.params = params;
    //Integrate
    *(jr+ii)= gsl_integration_glfixed (&JRInt,*(umin+ii),*(umax+ii),T)
      * sqrt(2.) * delta / M_PI;
  }
  gsl_integration_glfixed_table_free ( T );
}
void calcJz(int ndata,
	    double * jz,
	    double * vmin,
	    double * E,
	    double * Lz,
	    double * I3V,
	    double delta,
	    double * u0,
	    double * cosh2u0,
	    double * sinh2u0,
	    double * potupi2,
	    int nargs,
	    struct actionAngleArg * actionAngleArgs,
	    int order){
  int ii;
  gsl_function JzInt;
  struct JzStaeckelArg * params= (struct JzStaeckelArg *) malloc ( sizeof (struct JzStaeckelArg) );
  params->delta= delta;
  params->nargs= nargs;
  params->actionAngleArgs= actionAngleArgs;
  //Setup integrator
  gsl_integration_glfixed_table * T= gsl_integration_glfixed_table_alloc (order);
  JzInt.function = &JzStaeckelIntegrand;
  for (ii=0; ii < ndata; ii++){
    if ( *(vmin+ii) == -9999.99 ){
      *(jz+ii)= 9999.99;
      continue;
    }
    if ( (0.5 * M_PI - *(vmin+ii)) / M_PI * 2. < 0.000001 ){//circular
      *(jz+ii) = 0.;
      continue;
    }
    //Setup function
    params->E= *(E+ii);
    params->Lz22delta= 0.5 * *(Lz+ii) * *(Lz+ii) / delta / delta;
    params->I3V= *(I3V+ii);
    params->u0= *(u0+ii);
    params->cosh2u0= *(cosh2u0+ii);
    params->sinh2u0= *(sinh2u0+ii);
    params->potupi2= *(potupi2+ii);
    JzInt.params = params;
    //Integrate
    *(jz+ii)= gsl_integration_glfixed (&JzInt,*(vmin+ii),M_PI/2.,T)
      * 2 * sqrt(2.) * delta / M_PI;
  }
  gsl_integration_glfixed_table_free ( T );
}
void calcUminUmax(int ndata,
		  double * umin,
		  double * umax,
		  double * ux,
		  double * pux,
		  double * E,
		  double * Lz,
		  double * I3U,
		  double delta,
		  double * u0,
		  double * sinh2u0,
		  double * v0,
		  double * sin2v0,
		  double * potu0v0,
		  int nargs,
		  struct actionAngleArg * actionAngleArgs){
  int ii;
  double peps, meps;
  gsl_function JRRoot;
  struct JRStaeckelArg * params= (struct JRStaeckelArg *) malloc ( sizeof (struct JRStaeckelArg) );
  params->delta= delta;
  params->nargs= nargs;
  params->actionAngleArgs= actionAngleArgs;
  //Setup solver
  int status;
  int iter, max_iter = 100;
  const gsl_root_fsolver_type *T;
  gsl_root_fsolver *s;
  double u_lo, u_hi;
  T = gsl_root_fsolver_brent;
  s = gsl_root_fsolver_alloc (T);
  JRRoot.function = &JRStaeckelIntegrandSquared;
  for (ii=0; ii < ndata; ii++){
    //Setup function
    params->E= *(E+ii);
    params->Lz22delta= 0.5 * *(Lz+ii) * *(Lz+ii) / delta / delta;
    params->I3U= *(I3U+ii);
    params->u0= *(u0+ii);
    params->sinh2u0= *(sinh2u0+ii);
    params->v0= *(v0+ii);
    params->sin2v0= *(sin2v0+ii);
    params->potu0v0= *(potu0v0+ii);
    JRRoot.params = params;
    //Find starting points for minimum
    if ( fabs(GSL_FN_EVAL(&JRRoot,*(ux+ii))) < 0.0000001){ //we are at umin or umax
      peps= GSL_FN_EVAL(&JRRoot,*(ux+ii)+0.0000001);
      meps= GSL_FN_EVAL(&JRRoot,*(ux+ii)-0.0000001);
      if ( fabs(peps) < 0.00000001 && fabs(meps) < 0.00000001 ) {//circular
	*(umin+ii) = *(ux+ii);
	*(umax+ii) = *(ux+ii);
      }
      else if ( peps < 0. && meps > 0. ) {//umax
	*(umax+ii)= *(ux+ii);
	u_lo= 0.9 * (*(ux+ii) - 0.0000001);
	u_hi= *(ux+ii) - 0.00000001;
	while ( GSL_FN_EVAL(&JRRoot,u_lo) >= 0. && u_lo > 0.000000001){
	  u_hi= u_lo; //this makes sure that brent evaluates using previous
	  u_lo*= 0.9;
	}
	//Find root
	gsl_set_error_handler_off();
	status = gsl_root_fsolver_set (s, &JRRoot, u_lo, u_hi);
	if (status == GSL_EINVAL) {
	  *(umin+ii) = -9999.99;
	  *(umax+ii) = -9999.99;
	  continue;
	}
	iter= 0;
	do
	  {
	    iter++;
	    status = gsl_root_fsolver_iterate (s);
	    u_lo = gsl_root_fsolver_x_lower (s);
	    u_hi = gsl_root_fsolver_x_upper (s);
	    status = gsl_root_test_interval (u_lo, u_hi,
					     9.9999999999999998e-13,
					     4.4408920985006262e-16);
	  }
	while (status == GSL_CONTINUE && iter < max_iter);
	if (status == GSL_EINVAL) {
	  *(umin+ii) = -9999.99;
	  *(umax+ii) = -9999.99;
	  continue;
	}
	gsl_set_error_handler (NULL);
	*(umin+ii) = gsl_root_fsolver_root (s);
      }
      else if ( peps > 0. && meps < 0. ){//umin
	*(umin+ii)= *(ux+ii);
	u_lo= *(ux+ii) + 0.0000001;
	u_hi= 1.1 * (*(ux+ii) + 0.0000001);
	while ( GSL_FN_EVAL(&JRRoot,u_hi) >= 0. ) {
	  u_lo= u_hi; //this makes sure that brent evaluates using previous
	  u_hi*= 1.1;
	}
	//Find root
	gsl_set_error_handler_off();
	status = gsl_root_fsolver_set (s, &JRRoot, u_lo, u_hi);
	if (status == GSL_EINVAL) {
	  *(umin+ii) = -9999.99;
	  *(umax+ii) = -9999.99;
	  continue;
	}
	iter= 0;
	do
	  {
	    iter++;
	    status = gsl_root_fsolver_iterate (s);
	    u_lo = gsl_root_fsolver_x_lower (s);
	    u_hi = gsl_root_fsolver_x_upper (s);
	    status = gsl_root_test_interval (u_lo, u_hi,
					     9.9999999999999998e-13,
					     4.4408920985006262e-16);
	  }
	while (status == GSL_CONTINUE && iter < max_iter);
	if (status == GSL_EINVAL) {
	  *(umin+ii) = -9999.99;
	  *(umax+ii) = -9999.99;
	  continue;
	}
	gsl_set_error_handler (NULL);
	*(umax+ii) = gsl_root_fsolver_root (s);
      }
    }
    else {
      u_lo= 0.9 * *(ux+ii);
      u_hi= *(ux+ii);
      while ( GSL_FN_EVAL(&JRRoot,u_lo) >= 0. && u_lo > 0.000000001){
	u_hi= u_lo; //this makes sure that brent evaluates using previous
	u_lo*= 0.9;
      }
      u_hi= (u_lo < 0.9 * *(ux+ii)) ? u_lo / 0.9 / 0.9: *(ux+ii);
      //Find root
      gsl_set_error_handler_off();
      status = gsl_root_fsolver_set (s, &JRRoot, u_lo, u_hi);
      if (status == GSL_EINVAL) {
	*(umin+ii) = -9999.99;
	*(umax+ii) = -9999.99;
	continue;
      }
      iter= 0;
      do
	{
	  iter++;
	  status = gsl_root_fsolver_iterate (s);
	  u_lo = gsl_root_fsolver_x_lower (s);
	  u_hi = gsl_root_fsolver_x_upper (s);
	  status = gsl_root_test_interval (u_lo, u_hi,
					   9.9999999999999998e-13,
					   4.4408920985006262e-16);
	}
      while (status == GSL_CONTINUE && iter < max_iter);
      if (status == GSL_EINVAL) {
	*(umin+ii) = -9999.99;
	*(umax+ii) = -9999.99;
	continue;
      }
      gsl_set_error_handler (NULL);
      *(umin+ii) = gsl_root_fsolver_root (s);
      //Find starting points for maximum
      u_lo= *(ux+ii);
      u_hi= 1.1 * *(ux+ii);
      while ( GSL_FN_EVAL(&JRRoot,u_hi) > 0.) {
	u_lo= u_hi; //this makes sure that brent evaluates using previous
	u_hi*= 1.1;
      }
      u_lo= (u_hi > 1.1 * *(ux+ii)) ? u_hi / 1.1 / 1.1: *(ux+ii);
      //Find root
      gsl_set_error_handler_off();
      status = gsl_root_fsolver_set (s, &JRRoot, u_lo, u_hi);
      if (status == GSL_EINVAL) {
	*(umin+ii) = -9999.99;
	*(umax+ii) = -9999.99;
	continue;
      }
      iter= 0;
      do
	{
	  iter++;
	  status = gsl_root_fsolver_iterate (s);
	  u_lo = gsl_root_fsolver_x_lower (s);
	  u_hi = gsl_root_fsolver_x_upper (s);
	  status = gsl_root_test_interval (u_lo, u_hi,
					   9.9999999999999998e-13,
					   4.4408920985006262e-16);
	}
      while (status == GSL_CONTINUE && iter < max_iter);
      if (status == GSL_EINVAL) {
	*(umin+ii) = -9999.99;
	*(umax+ii) = -9999.99;
	continue;
      }
      gsl_set_error_handler (NULL);
      *(umax+ii) = gsl_root_fsolver_root (s);
    }
  }
 gsl_root_fsolver_free (s);    
}
void calcZmax(int ndata,
	      double * zmax,
	      double * z,
	      double * vz,
	      double * R,
	      double * Ez,
	      int nargs,
	      struct actionAngleArg * actionAngleArgs){
  int ii;
  gsl_function JzRoot;
  struct JzAdiabaticArg * params= (struct JzAdiabaticArg *) malloc ( sizeof (struct JzAdiabaticArg) );
  params->nargs= nargs;
  params->actionAngleArgs= actionAngleArgs;
  //Setup solver
  int status;
  int iter, max_iter = 100;
  const gsl_root_fsolver_type *T;
  gsl_root_fsolver *s;
  double z_lo, z_hi;
  T = gsl_root_fsolver_brent;
  s = gsl_root_fsolver_alloc (T);
  JzRoot.function = &JzAdiabaticIntegrandSquared;
  for (ii=0; ii < ndata; ii++){
    //Setup function
    params->Ez= *(Ez+ii);
    params->R= *(R+ii);
    JzRoot.params = params;
    //Find starting points for minimum
    if ( fabs(GSL_FN_EVAL(&JzRoot,*(z+ii))) < 0.0000001){ //we are at zmax
      *(zmax+ii)= *(z+ii);
    }
    else {
      z_lo= *(z+ii);
      z_hi= 1.1 * *(z+ii);
      while ( GSL_FN_EVAL(&JzRoot,z_lo) >= 0. ){
	z_lo= z_hi; //this makes sure that brent evaluates using previous
	z_hi*= 1.1;
      }
      //Find root
      gsl_set_error_handler_off();
      status = gsl_root_fsolver_set (s, &JzRoot, z_lo, z_hi);
      if (status == GSL_EINVAL) {
	*(zmax+ii) = -9999.99;
	continue;
      }
      iter= 0;
      do
	{
	  iter++;
	  status = gsl_root_fsolver_iterate (s);
	  z_lo = gsl_root_fsolver_x_lower (s);
	  z_hi = gsl_root_fsolver_x_upper (s);
	  status = gsl_root_test_interval (z_lo, z_hi,
					   9.9999999999999998e-13,
					   4.4408920985006262e-16);
	}
      while (status == GSL_CONTINUE && iter < max_iter);
      if (status == GSL_EINVAL) {
	*(zmax+ii) = -9999.99;
	continue;
      }
      gsl_set_error_handler (NULL);
      *(zmax+ii) = gsl_root_fsolver_root (s);
    }
  }
  gsl_root_fsolver_free (s);    
}

double JRStaeckelIntegrand(double u,
			   void * p){
  return sqrt(JRStaeckelIntegrandSquared(u,p));
}
double JRStaeckelIntegrandSquared(double u,
				  void * p){
  struct JRStaeckelArg * params= (struct JRStaeckelArg *) p;
  double sinh2u= sinh(u) * sinh(u);
  double dU= (sinh2u+params->sin2v0)
    *evaluatePotentialsUV(u,params->v0,params->delta,
			  params->nargs,params->actionAngleArgs)
    - (params->sinh2u0+params->sin2v0)*params->potu0v0;
  return params->E * sinh2u - params->I3U - dU  - params->Lz22delta / sinh2u;
}
  
double JzAdiabaticIntegrand(double z,
			    void * p){
  return sqrt(JzSAdiabaticIntegrandSquared(z,p));
}
double JzAdiabaticIntegrandSquared(double z,
				   void * p){
  struct JzAdiabaticArg * params= (struct JzAdiabaticArg *) p;
  return params->Ez - evaluateVerticalPotentials(params->R,z,
						 params->nargs,
						 params->actionAngleArgs);
}
double u0Equation(double u, void * p){
  struct u0EqArg * params= (struct u0EqArg *) p;
  double sinh2u= sinh(u) * sinh(u);
  double cosh2u= cosh(u) * cosh(u);
  double dU= cosh2u * evaluatePotentialsUV(u,0.5*M_PI,params->delta,
				    params->nargs,params->actionAngleArgs);
  return -(params->E*sinh2u-dU-params->Lz22delta/sinh2u);
}  
double evaluateVerticalPotentials(double R, double z,
				  int nargs, 
				  struct actionAngleArg * actionAngleArgs){
  return evaluatePotentials(R,z,nargs,actionAngleArgs)
    -evaluatePotentials(R,0.,nargs,actionAngleArgs);
}
