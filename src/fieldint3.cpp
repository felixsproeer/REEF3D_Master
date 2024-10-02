/*--------------------------------------------------------------------
REEF3D
Copyright 2008-2024 Hans Bihs

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

#include"fieldint3.h"
#include"lexer.h"

fieldint3::fieldint3(lexer *p)
{
    imin=p->imin;
    imax=p->imax;
    jmin=p->jmin;
    jmax=p->jmax;
    kmin=p->kmin;
    kmax=p->kmax;

	fieldalloc(p);
	
	pp=p;
}

fieldint3::~fieldint3()
{
	delete [ ] V;
}

void fieldint3::fieldalloc(lexer* p)
{
    int gridsize = p->imax*p->jmax*p->kmax;
	p->Iarray(V,gridsize);
}

void fieldint3::resize(lexer* p)
{
    int factor=3;
    
	p->Iresize(gcfeld,gcfeldsize, p->gcextra3*factor, 6, 6, 4, 4); 
	gcfeldsize=p->gcextra3*factor;
}

int & fieldint3::operator()(int ii, int jj, int kk)
{	
	return V[(ii-imin)*jmax*kmax + (jj-jmin)*kmax + kk-kmin];
}

