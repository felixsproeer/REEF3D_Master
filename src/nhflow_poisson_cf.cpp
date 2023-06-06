/*--------------------------------------------------------------------
REEF3D
Copyright 2008-2023 Hans Bihs

This file is part of REEF3D.

REEF3D is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, see <http://www.gnu.org/licenses/>.
--------------------------------------------------------------------
Author: Hans Bihs
--------------------------------------------------------------------*/

#include"nhflow_poisson_cf.h"
#include"lexer.h"
#include"fdm_nhf.h"
#include"heat.h"
#include"field.h"
#include"concentration.h"
#include"density_f.h"
#include"density_comp.h"
#include"density_conc.h"
#include"density_heat.h"
#include"density_vof.h"

nhflow_poisson_cf::nhflow_poisson_cf(lexer *p) 
{
    teta=0.5;
}

nhflow_poisson_cf::~nhflow_poisson_cf()
{
}

void nhflow_poisson_cf::start(lexer* p, fdm_nhf *d, field &f)
{	
    double sigxyz2;
   
	n=0;
    LOOP
	{
        if(p->wet[IJ]==1)
        {
            sigxyz2 = pow(0.5*(p->sigx[FIJK]+p->sigx[FIJKp1]),2.0) + pow(0.5*(p->sigy[FIJK]+p->sigy[FIJKp1]),2.0) + pow(p->sigz[IJ],2.0);
            
            
            d->M.p[n]  =  (CPORNH*PORVALNH)/(p->W1*p->DXP[IP]*p->DXN[IP])
                        + (CPORNH*PORVALNH)/(p->W1*p->DXP[IM1]*p->DXN[IP])
                        
                        + (CPORNH*PORVALNH)/(p->W1*p->DYP[JP]*p->DYN[JP])*p->y_dir
                        + (CPORNH*PORVALNH)/(p->W1*p->DYP[JM1]*p->DYN[JP])*p->y_dir
                        
                        + (sigxyz2*CPORNH*PORVALNH)/(p->W1*p->DZP[KP]*p->DZN[KP])
                        + (sigxyz2*CPORNH*PORVALNH)/(p->W1*p->DZP[KM1]*p->DZN[KP]);


            d->M.n[n] = -(CPORNH*PORVALNH)/(p->W1*p->DXP[IP]*p->DXN[IP]);
            d->M.s[n] = -(CPORNH*PORVALNH)/(p->W1*p->DXP[IM1]*p->DXN[IP]);

            d->M.w[n] = -(CPORNH*PORVALNH)/(p->W1*p->DYP[JP]*p->DYN[JP])*p->y_dir;
            d->M.e[n] = -(CPORNH*PORVALNH)/(p->W1*p->DYP[JM1]*p->DYN[JP])*p->y_dir;

            d->M.t[n] = -(sigxyz2*CPORNH*PORVALNH)/(p->W1*p->DZP[KP]*p->DZN[KP])     
                        + CPORNH*PORVALNH*0.5*(p->sigxx[FIJK]+p->sigxx[FIJKp1])/(p->W1*(p->DZN[KP]+p->DZN[KM1]));
                        
            d->M.b[n] = -(sigxyz2*CPORNH*PORVALNH)/(p->W1*p->DZP[KM1]*p->DZN[KP]) 
                        - CPORNH*PORVALNH*0.5*(p->sigxx[FIJK]+p->sigxx[FIJKp1])/(p->W1*(p->DZN[KP]+p->DZN[KM1]));
            
            
            d->rhsvec.V[n] +=  CPORNH*PORVALNH*(p->sigx[FIJK]+p->sigx[FIJKp1])*(f(i+1,j,k+1) - f(i-1,j,k+1) - f(i+1,j,k-1) + f(i-1,j,k-1))
                            /(p->W1*(p->DXN[IP]+p->DXN[IM1])*(p->DZN[KP]+p->DZN[KM1]))
                        
                            + CPORNH*PORVALNH*(p->sigy[FIJK]+p->sigy[FIJKp1])*(f(i,j+1,k+1) - f(i,j-1,k+1) - f(i,j+1,k-1) + f(i,j-1,k-1))
                            /((p->W1*p->DYN[JP]+p->DYN[JM1])*(p->DZN[KP]+p->DZN[KM1]))*p->y_dir;
        }
	
	++n;
	}
    
    n=0;
	LOOP
	{
        if(p->wet[IJ]==1)
        {
            if(p->flag4[Im1JK]<0)
            {
            d->rhsvec.V[n] -= d->M.s[n]*f(i-1,j,k);
            d->M.s[n] = 0.0;
            }
            
            if(p->flag4[Ip1JK]<0)
            {
            d->rhsvec.V[n] -= d->M.n[n]*f(i+1,j,k);
            d->M.n[n] = 0.0;
            }
            
            if(p->flag4[IJm1K]<0)
            {
            d->rhsvec.V[n] -= d->M.e[n]*f(i,j-1,k)*p->y_dir;
            d->M.e[n] = 0.0;
            }
            
            if(p->flag4[IJp1K]<0)
            {
            d->rhsvec.V[n] -= d->M.w[n]*f(i,j+1,k)*p->y_dir;
            d->M.w[n] = 0.0;
            }
            
            // BEDBC
            if(p->flag4[IJKm1]<0)
            {
            d->rhsvec.V[n] += d->M.b[n]*p->DZP[KM1]*d->WL(i,j)*p->W1*d->dwdt(i,j);
            d->M.p[n] += d->M.b[n];
            d->M.b[n] = 0.0;
            /*
            d->rhsvec.V[n] -= d->M.b[n]*f(i,j,k-1);
            d->M.b[n] = 0.0;*/
            }
            
            // FSFBC
            if(p->flag4[IJKp1]<0)
            {
                if(p->D37==1)
                {
                d->rhsvec.V[n] -= d->M.t[n]*f(i,j,k+1);
                d->M.t[n] = 0.0;
                }
                
                if(p->D37==2)
                {
                sigxyz2 = pow(0.5*(p->sigx[FIJK]+p->sigx[FIJKp1]),2.0) + pow(0.5*(p->sigy[FIJK]+p->sigy[FIJKp1]),2.0) + pow(p->sigz[IJ],2.0);
                
                d->M.p[n] -= (sigxyz2*CPORNH*PORVALNH)/(p->W1*p->DZP[KP]*p->DZN[KP]);
                d->M.p[n] += (sigxyz2*CPORNH*PORVALNH)/(p->W1*teta*p->DZP[KP]*p->DZN[KP]);
                        
                
                /*d->M.t[n] -= -(sigxyz2*CPOR3*PORVAL3)/(pd->roface(p,a,0,0,1)*p->DZP[KP]*p->DZN[KP])     
                        + CPORNH*PORVALNH*0.5*(p->sigxx[FIJK]+p->sigxx[FIJKp1])/(p->W1*(p->DZN[KP]+p->DZN[KM1]));
                        
                d->M.t[n] += -(sigxyz2*CPOR3*PORVAL3)/(pd->roface(p,a,0,0,1)*teta*p->DZP[KP]*p->DZN[KP])     
                        + CPORNH*PORVALNH*0.5*(p->sigxx[FIJK]+p->sigxx[FIJKp1])/(p->W1*(teta*p->DZN[KP]+p->DZN[KM1]));
                */
                d->M.p[n] += teta*d->M.t[n];
                d->M.t[n] = 0.0;
                
                d->rhsvec.V[n] -=  CPORNH*PORVALNH*(p->sigx[FIJK]+p->sigx[FIJKp1])*(f(i+1,j,k+1) - f(i-1,j,k+1) - f(i+1,j,k-1) + f(i-1,j,k-1))
                        /(p->W1*(p->DXN[IP]+p->DXN[IM1])*(p->DZN[KP]+p->DZN[KM1]))
                        
                        + CPORNH*PORVALNH*(p->sigy[FIJK]+p->sigy[FIJKp1])*(f(i,j+1,k+1) - f(i,j-1,k+1) - f(i,j+1,k-1) + f(i,j-1,k-1))
                        /((p->W1*p->DYN[JP]+p->DYN[JM1])*(p->DZN[KP]+p->DZN[KM1]))*p->y_dir;
                        
                d->rhsvec.V[n] +=  CPORNH*PORVALNH*(p->sigx[FIJK]+p->sigx[FIJKp1])*( - f(i+1,j,k-1) + f(i-1,j,k-1))
                        /(p->W1*(p->DXN[IP]+p->DXN[IM1])*(teta*p->DZN[KP]+p->DZN[KM1]))
                        
                        + CPORNH*PORVALNH*(p->sigy[FIJK]+p->sigy[FIJKp1])*( - f(i,j+1,k-1) + f(i,j-1,k-1))
                        /((p->W1*p->DYN[JP]+p->DYN[JM1])*(teta*p->DZN[KP]+p->DZN[KM1]))*p->y_dir;
                }
                
                if(p->D37==3)
                {
                sigxyz2 = pow(0.5*(p->sigx[FIJK]+p->sigx[FIJKp1]),2.0) + pow(0.5*(p->sigy[FIJK]+p->sigy[FIJKp1]),2.0) + pow(p->sigz[IJ],2.0);
                
                double x0,x1,x2,y2;
                double x,y;
                double Lx0,Lx1,Lx2;
                
                x0 = -0.5*p->DZP[KP]*p->sigz[IJ] - 0.5*p->DZP[KM1]*p->sigz[IJ];
                x1 = -0.5*p->DZP[KP]*p->sigz[IJ];
                x2 = 0.0;


                x = 0.5*p->DZP[KP]*p->sigz[IJ];

                Lx0 = ((x-x1)/(x0-x1)) * ((x-x2)/(x0-x2));
                Lx1 = ((x-x0)/(x1-x0)) * ((x-x2)/(x1-x2));
                Lx2 = ((x-x0)/(x2-x0)) * ((x-x1)/(x2-x1));

                //d->rhsvec.V[n]  -= d->M.t[n]*Lx2*0.0;
                d->M.p[n]       += d->M.t[n]*Lx1;
                d->M.b[n]       += d->M.t[n]*Lx0;
                d->M.t[n]       = 0.0;
                
                /*d->M.p[n] -= (sigxyz2*CPOR3*PORVAL3)/(pd->roface(p,a,0,0,1)*p->DZP[KP]*p->DZN[KP]);
                d->M.p[n] += (sigxyz2*CPOR3*PORVAL3)/(pd->roface(p,a,0,0,1)*teta*p->DZP[KP]*p->DZN[KP]);
                           
                d->M.t[n] = 0.0;*/
                
                
                d->rhsvec.V[n] -=  CPORNH*PORVALNH*(p->sigx[FIJK]+p->sigx[FIJKp1])*(f(i+1,j,k+1) - f(i-1,j,k+1) - f(i+1,j,k-1) + f(i-1,j,k-1))
                        /(p->W1*(p->DXN[IP]+p->DXN[IM1])*(p->DZN[KP]+p->DZN[KM1]))
                        
                        + CPORNH*PORVALNH*(p->sigy[FIJK]+p->sigy[FIJKp1])*(f(i,j+1,k+1) - f(i,j-1,k+1) - f(i,j+1,k-1) + f(i,j-1,k-1))
                        /((p->W1*p->DYN[JP]+p->DYN[JM1])*(p->DZN[KP]+p->DZN[KM1]))*p->y_dir;
                        
                d->rhsvec.V[n] +=  CPORNH*PORVALNH*(p->sigx[FIJK]+p->sigx[FIJKp1])*( - f(i+1,j,k-1) + f(i-1,j,k-1))
                        /(p->W1*(p->DXN[IP]+p->DXN[IM1])*(teta*p->DZN[KP]+p->DZN[KM1]))
                        
                        + CPORNH*PORVALNH*(p->sigy[FIJK]+p->sigy[FIJKp1])*( - f(i,j+1,k-1) + f(i,j-1,k-1))
                        /((p->W1*p->DYN[JP]+p->DYN[JM1])*(teta*p->DZN[KP]+p->DZN[KM1]))*p->y_dir;
                }
                
            }
        }
	++n;
	}
}