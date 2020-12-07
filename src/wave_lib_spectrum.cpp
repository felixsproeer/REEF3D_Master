/*--------------------------------------------------------------------
REEF3D
Copyright 2008-2020 Hans Bihs

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

#include"wave_lib_spectrum.h"
#include"lexer.h"
#include"ghostcell.h"
#include<iostream>
#include<fstream>
#include<sys/stat.h>
#include<sys/types.h>

wave_lib_spectrum::wave_lib_spectrum()
{
}

wave_lib_spectrum::~wave_lib_spectrum()
{
}

double wave_lib_spectrum::wave_spectrum(lexer *p, double w)
{
	if(p->B85==1)
    Sval = PM(p,w);

    if(p->B85==2)
    Sval = JONSWAP(p,w);

    if(p->B85==3)
    Sval = Torsethaugen(p,w);


    if(p->B85==21)
    Sval = Goda_JONSWAP(p,w);

	if(p->B85==10)
    Sval = spectrum_file(p,w);

    return Sval;
}

void wave_lib_spectrum::irregular_parameters(lexer *p)
{
	if(p->B85==10)
	spectrum_file_read(p);


    double maxS=-1.0;
	double S,w,sigma;
	int check_s,check_e;
    int n;

    p->wN = p->B86;

	w=0.0;
	wp=0.0;

	// Find spectrum peak
	do{
	w+=0.01;

	if(w<=p->wwp)
    {
        sigma=0.07;
    }

	if(w>p->wwp)
    {
        sigma=0.09;
    }

	 S = wave_spectrum(p,w);

	 if(S>maxS)
     {
        wp=w;
     }

	 maxS = MAX(S,maxS);

	}while(w<100.0);

	if(p->mpirank==0)
	cout<<"maxS: "<<maxS<<"  wp: "<<wp<<"  wwp: "<<p->wwp<<endl;

	// find omega_start and omega_end
	check_s=0;
	check_e=0;
	w=0.0;
	do{
        w+=0.01;

        if(w<=p->wwp)
        {
        sigma=0.07;
        }

        if(w>p->wwp)
        {
        sigma=0.09;
        }

        S = wave_spectrum(p,w);

        if(w<wp && S>0.05*maxS && check_s==0)
        {
            ws = w;
            check_s=1;
        }

        if(w>=wp && S<=0.1*maxS && check_e==0)
        {
            we = w;
            check_e=1;
        }

	}while(w<100.0);

	if(p->B87==1)
	{
        ws=p->B87_1;
        we=p->B87_2;
	}

	if(p->B130 == 0)
    {
    numcomp=p->wN;
    }

    if(p->B130 > 0 && p->B136 != 4)
    {
    numcomp=p->wN*p->B133;
    }

    if(p->B130 > 0 && p->B136 == 4)
    {
    numcomp=p->wN;
    }

    p->Darray(Si,numcomp);
    p->Darray(wi,numcomp);
    p->Darray(dw,numcomp);
    p->Darray(Ai,numcomp);
    p->Darray(Li,numcomp);
    p->Darray(ki,numcomp);
    p->Darray(Ti,numcomp);
    p->Darray(ei,numcomp);
    p->Darray(beta,numcomp);
    p->Darray(cosbeta,numcomp);
    p->Darray(sinbeta,numcomp);

    // Peak Enhance Method

    if(p->B84==1)
    {

        for(n=0;n<numcomp;++n)
        {
            beta[n]=0.0;
            sinbeta[n]=0.0;
            cosbeta[n]=1.0;
        }

        // find dw and maintain wp
        double ds,de,dd;
        int wNs,wNe;

        dd=(we-ws)/double(p->wN);
        ds = wp-ws;
        de = we-wp;

        wNs = int((ds/(we-ws))*double(p->wN));
        wNe = int((de/(we-ws))*double(p->wN));

        if(wNs+wNe<p->wN)
        {
            if(wNs<=wNe)
                wNs+=1;

            if(wNs>wNe)
                wNe+=1;
        }

        if(wNs+wNe>p->wN)
        {
            if(wNs<=wNe)
                wNe-=1;

            if(wNs>wNe)
                wNs-=1;
        }


        for(n=0;n<wNs;++n)
        dw[n] = (wp-ws)/double(wNs);

        for(n=wNs;n<p->wN;++n)
        dw[n] = (we-wp)/double(wNe);

        if(p->mpirank==0)
        cout<<"wNs: "<<wNs<<"  wNe: "<<wNe<<"  p->wN "<<p->wN<<endl;

        if(p->mpirank==0)
        cout<<"ws: "<<ws<<"  we: "<<we<<endl;

        w=ws;
        for(n=0;n<p->wN;++n)
        {
            wi[n]=w;
            w+=dw[n];
        }

    }

    // Equal Energy Method
    if(p->B84==2)
    {
        double ddw, sum;
        double w_low, w_high, cdf_low, cdf_high;
        int m, NN;

        ws=0.75*wp;
        we=2.0*wp;

       for(n=0;n<numcomp;++n)
        {
            beta[n]=0.0;
            sinbeta[n]=0.0;
            cosbeta[n]=1.0;
        }

        // Integration of spectrum

        ddw=0.01;
        NN=int((we-ws)/ddw)+1;

        p->Darray(Sn,NN);
        p->Darray(cdf,NN);
        p->Darray(ww,NN);

        w=ws;
        for(n=0;n<NN;++n)
        {
            ww[n]=w;
            S = wave_spectrum(p,w);
            Sn[n]=S;
            w+=ddw;
        }

        sum=0.0;
        for(n=0;n<NN;++n)
        {
            sum+=ddw*Sn[n];
            cdf[n]=sum;
        }

        // Create equal energy bins

        p->Darray(dee,p->wN);

        for(n=0;n<p->wN;++n)
        {
            dee[n] = cdf[0]+n*(cdf[NN-1]-cdf[0])/double (p->wN-1);
        }

        // Interpolate the corresponding frequencies and frequency intervals at each equal energy bins
        for(n=0;n<p->wN;++n)
        {
            for(m=0;m<NN;++m)
            {
                if(cdf[m]<=dee[n])
                {
                    cdf_low=cdf[m];
                    w_low=ww[m];
                }
            }
            for(m=(NN-1);m>=0;--m)
            {
                if(Sn[m]>=dee[n])
                {
                    cdf_high=cdf[m];
                    w_high=ww[m];
                }
            }
            if(w_low==w_high)
            {
                wi[n]=w_low;
            }

            if(w_low!=w_high)
            {
                wi[n]=(dee[n]-cdf_low)*(w_high-w_low)/(cdf_high-cdf_low)+w_low;
            }
            Si[n] = wave_spectrum(p,wi[n]);
            dw[n]=(cdf[NN-1]-cdf[0])/p->wN/Si[n];
        }
    }

        // Final step: fill Si and the corresponding values for Ai and ki
        for(n=0;n<p->wN;++n)
        {
            w=wi[n];
            Si[n] = wave_spectrum(p,w);
            wL0 = (2.0*PI*9.81)/pow(w,2.0);
            k0 = (2.0*PI)/wL0;
            S0 = sqrt(k0*p->wd) * (1.0 + (k0*p->wd)/6.0 + (k0*k0*p->wd*p->wd)/30.0);
            Li[n] = wL0*tanh(S0);

            for(int qn=0; qn<100; ++qn)
            {
                Li[n] = wL0*tanh(2.0*PI*p->wd/Li[n]);
            }

            ki[n] = 2.0*PI/Li[n];
        }

        print_spectrum(p);

        // directional spreading
        directional_spreading(p);
}

void wave_lib_spectrum::amplitudes_irregular(lexer *p)
{
    if(p->B84==1 && p->B136!=4)
    {
        // Amplitudes
        for(int n=0;n<p->wN;++n)
        {
            Ai[n] = sqrt(2.0*Si[n]*dw[n]*dbeta);
        }
    }

    if(p->B84==2 && p->B136==4)
    {
        // Amplitudes
        for(int n=0;n<p->wN;++n)
        {
            Ai[n] = sqrt(2.0*Si[n]*dw[n]);
        }
    }
}

void wave_lib_spectrum::amplitudes_focused(lexer *p)
{
    int n;
    double Sw_sum;

    // Amplitudes
	if(p->B82==1 || p->B82==11)
	{
    Sw_sum=0.0;

	for(n=0;n<p->wN;++n)
	Sw_sum+=Si[n]*dw[n];

	for(n=0;n<p->wN;++n)
	Ai[n] = (p->wAs*Si[n]*dw[n])/Sw_sum;
	}

    if(p->B82==2 || p->B82==12)
	for(n=0;n<p->wN;++n)
	Ai[n] = sqrt(2.0*Si[n]*dw[n]);

	if(p->B82==3 || p->B82==13)
	for(n=0;n<p->wN;++n)
	Ai[n] = (2.0*PI*p->B83)/(ki[n]);

	if(p->B82==4 || p->B82==14)
	for(n=0;n<p->wN;++n)
	Ai[n] = p->wAs;

}

void wave_lib_spectrum::phases_irregular(lexer *p)
{
    if(p->B139>0)
    srand(p->B139);
    
    if(p->B139==0)
    srand((unsigned)time(0));

    // make phases
	for(int n=0;n<p->wN;++n)
	ei[n]  = double(rand() % 628)/100.0;
}

void wave_lib_spectrum::phases_focused(lexer *p)
{
    // make phases
    if(p->B130 == 0)
    {
        if(p->B82<11)
        {
	    	for(int n=0;n<p->wN;++n)
				{
				ei[n]  = ki[n]*p->B81_1 - wi[n]*p->B81_2;
				}
	    }
	    if(p->B82>=11)
			{
	    	for(int n=0;n<p->wN;++n)
				{
				ei[n]  = ki[n]*p->B81_1 - wi[n]*p->B81_2 + PI;
				}
			}
    }

    if(p->B130 > 0)
    {
        for(int n=0;n<numcomp;++n)
        {
        ei[n]  = ki[n]*cosbeta[n]*p->B81_1 + ki[n]*sinbeta[n]*p->B81_3 - wi[n]*p->B81_2;
        }
    }
}

void wave_lib_spectrum::print_spectrum(lexer *p)
{
	ofstream result;

	double xval=ws;

	// Create Folder
	if(p->mpirank==0 && p->P14==1)
	mkdir("./REEF3D_Log",0777);

    if(p->mpirank==0)
    {
    // open file
	if(p->P14==0)
    result.open("REEF3D_wave-spectrum.dat");

	if(p->P14==1)
	result.open("./REEF3D_Log/REEF3D_wave-spectrum.dat");
	}

	for(int n=0;n<p->wN;++n)
	{
	xval+=dw[n];
	result<<xval<<" "<<Si[n]<<endl;
	}

	result.close();
}

void wave_lib_spectrum::print_components(lexer *p)
{
	ofstream result;

	double xval=ws;

	// Create Folder
	if(p->mpirank==0 && p->P14==1)
	mkdir("./REEF3D_Log",0777);

    if(p->mpirank==0)
    {
    // open file
	if(p->P14==0)
    result.open("REEF3D_wave-components.dat");

	if(p->P14==1)
	result.open("./REEF3D_Log/REEF3D_wave-components.dat");
	}

	for(int n=0;n<p->wN;++n)
	{
	xval+=dw[n];
	result<<Ai[n]<<" "<<wi[n]<<" "<<ei[n]<<endl;
	}

	result.close();
}