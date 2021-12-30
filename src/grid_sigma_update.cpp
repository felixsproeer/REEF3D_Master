/*--------------------------------------------------------------------
REEF3D
Copyright 2008-2021 Hans Bihs

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
--------------------------------------------------------------------*/

#include"grid_sigma.h"
#include"lexer.h"
#include"fdm.h"
#include"ghostcell.h"
#include"fnpf_ddx_cds2.h"
#include"fnpf_ddx_cds4.h"
#include"fnpf_cds2.h"
#include"fnpf_cds4.h"
#include"grid_sigma_data.h"

#define WLVL (fabs(a->WL(i,j))>1.0e-20?a->WL(i,j):1.0-20)


void grid_sigma::sigma_update(lexer *p, fdm *a, ghostcell *pgc, slice &eta, slice &eta_n, double alpha)
{
    double wl;
    
    // calculate: Ex,Ey,Exx,Eyy
    // 3D
    if(p->i_dir==1 && p->j_dir==1)
    SLICELOOP4
    {
    pd->Ex(i,j) = pdx->sx(p,eta,1.0);
    pd->Ey(i,j) = pdx->sy(p,eta,1.0);
    
    pd->Exx(i,j) = pddx->sxx(p,eta);
    pd->Eyy(i,j) = pddx->syy(p,eta);
    }
    
    // 2D
    if(p->j_dir==0)
    SLICELOOP4
    {
    pd->Ex(i,j) = pdx->sx(p,eta,1.0);    
    pd->Exx(i,j) = pddx->sxx(p,eta);
    }
    
    pgc->gcsl_start4(p,pd->Ex,1);
    pgc->gcsl_start4(p,pd->Ey,1);
    
    // calculate: Bx,By,Bxx,Byy
    // 3D
    if(p->j_dir==1)
    SLICELOOP4
    {
    pd->Bx(i,j) = pdx->sx(p,a->depth,1.0);
    pd->By(i,j) = pdx->sy(p,a->depth,1.0);
    
    pd->Bxx(i,j) = pddx->sxx(p,a->depth);
    pd->Byy(i,j) = pddx->syy(p,a->depth);
    }

    // 2D
    if(p->j_dir==0)
    SLICELOOP4
    {
    pd->Bx(i,j) = pdx->sx(p,a->depth,1.0);    
    pd->Bxx(i,j) = pddx->sxx(p,a->depth);
    }
    
    pgc->gcsl_start4(p,pd->Bx,1);
    pgc->gcsl_start4(p,pd->By,1);
    
    
    // updwind
    /*double Pval,Qval;
    
    SLICELOOP4
    {
    Pval = 0.5*(a->P(i-1,j)+a->P(i,j));
    Qval = 0.5*(a->Q(i,j-1)+a->Q(i,j));
    
    if(Pval>=0.0)
    {
    pd->Ex(i,j) = (eta(i,j)-eta(i-1,j))/p->DXP[IM1];    
    pd->Bx(i,j) = (a->depth(i,j)-a->depth(i-1,j))/p->DXP[IM1];   
    }
    
    if(Pval<0.0)
    {
    pd->Ex(i,j) = (eta(i+1,j)-eta(i,j))/p->DXP[IP];    
    pd->Bx(i,j) = (a->depth(i+1,j)-a->depth(i,j))/p->DXP[IP];   
    }
    
    }*/
    
    // sigx
    FLOOP
    p->sigx[FIJK] = (1.0 - p->sig[FIJK])*(pd->Bx(i,j)/WLVL) - p->sig[FIJK]*(pd->Ex(i,j)/WLVL);
    
    // sigy
    FLOOP
    p->sigy[FIJK] = (1.0 - p->sig[FIJK])*(pd->By(i,j)/WLVL) - p->sig[FIJK]*(pd->Ey(i,j)/WLVL);
    
    // sigz
    //SLICELOOP4
    //p->sigz[IJ] = 1.0/WLVL;
    
    SLICELOOP4
    {
    wl=MAX(0.0, eta(i,j) + p->wd - a->bed(i,j));
    
    wl = (fabs(wl)>1.0e-20?wl:1.0-20);
    
    p->sigz[IJ] = 1.0/wl;
    }
    

    
    // sigt
    FLOOP
    {
    p->sigt[FIJK] = -(p->sig[FIJK]/WLVL)*(a->WL(i,j)-a->WL_n(i,j))/(p->dt);
    }
    
    // sigxx
    FLOOP
    {
        p->sigxx[FIJK] = ((1.0 - p->sig[FIJK])/WLVL)*(pd->Bxx(i,j) - pow(pd->Bx(i,j),2.0)/WLVL) // xx
        
                      - (p->sig[FIJK]/WLVL)*(pd->Exx(i,j) - pow(pd->Ex(i,j),2.0)/WLVL)
                      
                      - (p->sigx[FIJK]/WLVL)*(pd->Bx(i,j) + pd->Ex(i,j))
                      
                      - ((1.0 - 2.0*p->sig[FIJK])/pow(WLVL,2.0))*(pd->Bx(i,j)*pd->Ex(i,j))
                      
                      
                      + ((1.0 - p->sig[FIJK])/WLVL)*(pd->Byy(i,j) - pow(pd->By(i,j),2.0)/WLVL) // yy
        
                      - (p->sig[FIJK]/WLVL)*(pd->Eyy(i,j) - pow(pd->Ey(i,j),2.0)/WLVL)
                      
                      - (p->sigy[FIJK]/WLVL)*(pd->By(i,j) + pd->Ey(i,j))
                      
                      - ((1.0 - 2.0*p->sig[FIJK])/pow(WLVL,2.0))*(pd->By(i,j)*pd->Ey(i,j));
    }
    
    
    // sig BC
    SLICELOOP4
    {
        k=0;
        if(p->nb5==-2)
        {
            p->sigx[FIJKm1] = p->sigx[KP];
            p->sigx[FIJKm2] = p->sigx[KP];
            p->sigx[FIJKm3] = p->sigx[KP];
        }
        
        k=p->knoz;
        if(p->nb6==-2)
        {
            p->sigx[FIJKp1] = p->sigx[KP];
            p->sigx[FIJKp2] = p->sigx[KP];
            p->sigx[FIJKp3] = p->sigx[KP];
        } 
    }
    
    
    SLICELOOP4
    {
        k=0;
        if(p->nb5==-2)
        {
            p->sigy[FIJKm1] = p->sigy[KP];
            p->sigy[FIJKm2] = p->sigy[KP];
            p->sigy[FIJKm3] = p->sigy[KP];
        }
        
        k=p->knoz;
        if(p->nb6==-2)
        {
            p->sigy[FIJKp1] = p->sigy[KP];
            p->sigy[FIJKp2] = p->sigy[KP];
            p->sigy[FIJKp3] = p->sigy[KP];
        } 
    }
    
    SLICELOOP4
    {
        k=0;
        if(p->nb5==-2)
        {
            p->sigxx[FIJKm1] = p->sigxx[KP];
            p->sigxx[FIJKm2] = p->sigxx[KP];
            p->sigxx[FIJKm3] = p->sigxx[KP];
        }
        
        k=p->knoz;
        if(p->nb6==-2)
        {
            p->sigxx[FIJKp1] = p->sigxx[KP];
            p->sigxx[FIJKp2] = p->sigxx[KP];
            p->sigxx[FIJKp3] = p->sigxx[KP];
        } 
    }
    /*
    k=p->knoz;
    SLICELOOP4
    {
        if(p->flag4[Im1JK]<0 || i==0)
        p->sigz[Im1J] = p->sigz[IJ];
        
        if(p->flag4[Ip1JK]<0 || i==p->knox-1)
        p->sigz[Ip1J] = p->sigz[IJ];
        
        if(p->flag4[IJm1K]<0 || j==0)
        p->sigz[IJm1] = p->sigz[IJ];
        
        if(p->flag4[IJp1K]<0 || j==p->knoy-1)
        p->sigz[IJp1] = p->sigz[IJ];
    }*/
    
    
    FLOOP
    p->ZSN[FIJK] = p->ZN[KP]*a->WL(i,j) + a->bed(i,j);
    
    
    LOOP
    p->ZSP[IJK]  = p->ZP[KP]*a->WL(i,j) + a->bed(i,j);


    pgc->start7S(p,p->sigx,1);
    pgc->start7S(p,p->sigy,1);
    pgc->start7S(p,p->sigxx,1);
    pgc->start7S(p,p->sigt,1);
    pgc->start7S(p,p->ZSN,1);
    pgc->gcslparaxijk(p, p->sigz, 1);
    
    //LOOP
    //a->test(i,j,k) = p->sigx[FIJKp1];
        
}

void grid_sigma::omega_update(lexer *p, fdm *a, ghostcell *pgc, field &u, field &v, field &w, slice &eta, slice &eta_n, double alpha)
{ 
    double wval;
    
    WLOOP
    {
    a->omega(i,j,k) =  p->sigt[FIJKp1]
                    
                    +  0.25*(u(i-1,j,k) + u(i-1,j,k+1) + u(i,j,k) + u(i,j,k+1))*p->sigx[FIJKp1]
                    
                    +  0.25*(v(i,j-1,k) + v(i,j-1,k+1) + v(i,j,k) + v(i,j,k+1))*p->sigy[FIJKp1]
                    
                    +  w(i,j,k)*p->sigz[IJ];
                    
    }
    
    GC3LOOP
    if(p->gcb3[n][3]==6 && p->gcb3[n][4]==3)
    {
    i=p->gcb3[n][0];
    j=p->gcb3[n][1];
    k=p->gcb3[n][2];
    
        for(int q=0;q<3;++q)
        a->omega(i,j,k+1+q) =   p->sigt[FIJKp2]
                    
                    +  0.5*(u(i-1,j,k+1) + u(i,j,k+1))*p->sigx[FIJKp2]
                    
                    +  0.5*(v(i,j-1,k+1) + v(i,j,k+1))*p->sigy[FIJKp2]
                    
                    +  w(i,j,k+1)*p->sigz[IJ];
    }
    
    pgc->start3(p,a->omega,17);
    pgc->start3(p,a->omega,17);
}


