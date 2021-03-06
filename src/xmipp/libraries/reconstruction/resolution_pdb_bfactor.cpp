/***************************************************************************
 *
 * Authors:    Jose Luis Vilas, 					  jlvilas@cnb.csic.es
 *
 * Unidad de  Bioinformatica of Centro Nacional de Biotecnologia , CSIC
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 * 02111-1307  USA
 *
 *  All comments concerning this program package may be sent to the
 *  e-mail address 'xmipp@cnb.csic.es'
 ***************************************************************************/

#include "resolution_pdb_bfactor.h"
#include <core/bilib/kernel.h>
#include "data/pdb.h"
#include <numeric>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <string.h>


void ProgResBFactor::readParams()
{
	fn_pdb = getParam("--atmodel");
	fn_locres = getParam("--vol");
	sampling = getDoubleParam("--sampling");
	medianTrue = checkParam("--median");
	fscResolution = getDoubleParam("--fscResolution");
	fnOut = getParam("-o");
}


void ProgResBFactor::defineParams()
{
	addUsageLine("The matching between a b-factor of an atomic model and the local resolution of a cryoEM map is analyzed.");
	addParamsLine("  --atmodel <vol_file=\"\">   		: Atomic model (pdb)");
	addParamsLine("  --vol <vol_file=\"\">				: Local resolution map");
	addParamsLine("  [--sampling <sampling=1>]			: Sampling Rate (A)");
	addParamsLine("  [--median]			                : The resolution an bfactor per residue are averaged instead of computed the median");
	addParamsLine("  [--fscResolution <fscResolution=-1>]			        : If this is provided, the FSC resolution in Angstrom is used to normalized");
	//addParamsLine("                                       the resolution as (LR-R)/R, where LR is the local resoluion and R is the global resolution");
	addParamsLine("  -o <output=\"amap.mrc\">			: Output of the algorithm");
}


void ProgResBFactor::analyzePDB()
{
	//Open the pdb file
	std::ifstream f2parse;
	f2parse.open(fn_pdb.c_str());

	double maxx=-1e-38, maxy, maxz, minx = 1e-38, miny, minz;
	maxy = maxx;
	maxz = maxx;
	miny = minx;
	minz = minx;

	numberOfAtoms = 0;

	int last_resi = 0;

	while (!f2parse.eof())
	{
		std::string line;
		getline(f2parse, line);

		// The type of record (line) is defined in the first 6 characters of the pdb
		std::string typeOfline = line.substr(0,4);

		if ( (typeOfline == "ATOM") || (typeOfline == "HETA"))
		{
			// Type of Atom
			std::string at = line.substr(13,2);

			if (at == "CA")
			{

				numberOfAtoms++;
				double x = textToFloat(line.substr(30,8));
				double y = textToFloat(line.substr(38,8));
				double z = textToFloat(line.substr(46,8));

				// The furthest points along each axis are found to set the boxsize with the sampling
				if (x<minx)
					minx = x;
				if (y<miny)
					miny = y;
				if (z<minz)
					minz = z;
				if (x>maxx)
					maxx = x;
				if (y>maxy)
					maxy = y;
				if (z>maxz)
					maxz = z;

				// Getting coordinates
				at_pos.x.push_back(x);
				at_pos.y.push_back(y);
				at_pos.z.push_back(z);

				int resi = (int) textToFloat(line.substr(23,5));

				at_pos.residue.push_back(resi);

				// Getting the bfactor =8pi^2*u
				double bfactorRad = sqrt(textToFloat(line.substr(60,6))/(8*PI*PI));
				at_pos.b.push_back(bfactorRad);

				double rad = atomCovalentRadius(line.substr(13,2));

				at_pos.atomCovRad.push_back(rad);
			}
		}
	}
}


template <typename T>
std::vector<size_t> ProgResBFactor::sort_indexes(const std::vector<T> &v)
{
  // initialize original index locations
  std::vector<size_t> idx(v.size());
  std::iota(idx.begin(), idx.end(), 0);

  // sort indexes based on comparing values in v
  // using std::stable_sort instead of std::sort
  // to avoid unnecessary index re-orderings
  // when v contains elements of equal values
  stable_sort(idx.begin(), idx.end(),
       [&v](size_t i1, size_t i2) {return v[i1] < v[i2];});

  return idx;
}


void ProgResBFactor::sweepByResidue(MultidimArray<int> &mask, std::vector<double> &residuesToChimera)
{
	// Reading Local Resolution Map
	Image<double> imgResVol;
	imgResVol.read(fn_locres);
	MultidimArray<double> resvol;
	resvol = imgResVol();

	// Getting dimensions of th local resolution map to match atom position and local resolution information
	size_t xdim, ydim, zdim, ndim;
	resvol.getDimensions(xdim, ydim, zdim, ndim);

	if (zdim == 1)
		zdim = ndim;

	// idx_residue determines the positional index of the residue
	// Note that residues are stored in the vector at_pos.residue.
	// Then, the vector at_pos.residue is sorted from low to high
	// The new vector idx_residue, determine the position of each
	// residue in the original vector.
	std::vector<size_t> idx_residue;
	idx_residue = sort_indexes(at_pos.residue);

	// The resolution of each residue is stored in each component (as many as components as residues)
	std::vector<double> resolution_per_residue(0);

	// The contributing voxels to a single residue are stored
	std::vector<double> resolution_to_estimate(0);

	// the b-factor per residue
	std::vector<double> bfactor_per_residue(0), resNumberList(0);

	// vector to estimate the bfactor of each residue
	std::vector<double> bfactor_to_estimate(0);

	// vectors to estimate the moving average
	std::vector<double> ma_l(0);
	std::vector<double> ma_c(0);

	// Setting the first residue before the loop that sweeps all residues
	size_t r = 0;
	size_t idx = idx_residue[r];

	// The b-factor and resolution mean per residue, as well as number of voxels contributing to the mean
	double bfactor_mean = 0;
	double resolution_mean = 0;
	int N_elems = 0;

	MetaData md;
	size_t objId;

	std::cout << "numberOfAtoms" << numberOfAtoms << std::endl;

	// Selecting the residue
	int resi, last_resi;

	size_t first_index = 0;
	size_t last_index = idx_residue.size()-1;

	//std::cout << "last_index = " << at_pos.residue[idx_residue[last_index]] << std::endl;

	last_resi = at_pos.residue[idx_residue[first_index]];

	for (size_t r=first_index; r<numberOfAtoms; ++r)
	{
		//std::cout << idx_residue[r] << std::endl;
		// Index of the residue in the residue vector
		idx = idx_residue[r];

		// Selecting the residue
		resi = at_pos.residue[r];

		if (resi != last_resi)
		{

			//std::cout << "New residue "<< resi << std::endl;


			double res_resi, bfactor_resi;
			//std::cout << resolution_to_estimate.size() << " " << size_t(resolution_to_estimate.size()*0.5) << std::endl;


			if (medianTrue)
			{
				std::sort(resolution_to_estimate.begin(), resolution_to_estimate.end());
				std::sort(bfactor_to_estimate.begin(), bfactor_to_estimate.end());

				res_resi = resolution_to_estimate[size_t(resolution_to_estimate.size()*0.5)];
				bfactor_resi = bfactor_to_estimate[size_t(bfactor_to_estimate.size()*0.5)];

			}else
			{
				res_resi = resolution_mean/N_elems;
				bfactor_resi = bfactor_mean/bfactor_to_estimate.size();
			}

			if (fscResolution>0)
			{
				res_resi -= fscResolution;
				res_resi = fscResolution;
			}


			resolution_per_residue.push_back(res_resi);
			bfactor_per_residue.push_back(bfactor_resi);

			ma_l.push_back(res_resi*0.3);
			ma_c.push_back(res_resi*0.4);
			resNumberList.push_back(last_resi);

//			objId = md.addObject();
//			md.setValue(MDL_BFACTOR, bfactor_resi, objId);
//			md.setValue(MDL_RESIDUE, last_resi, objId);
//			md.setValue(MDL_RESOLUTION_LOCAL_RESIDUE, res_resi, objId);
			//std::cout << last_resi << "  " << res_resi << ";" << std::endl;

			last_resi = resi;

			// The means are reseted
			bfactor_mean = 0;
			resolution_mean = 0;
			N_elems = 0;
			resolution_to_estimate.clear();
			bfactor_to_estimate.clear();
		}

		// Getting the atom position
		int k = round(at_pos.z[idx]/sampling) + floor(zdim/2);
		int i = round(at_pos.y[idx]/sampling) + floor(ydim/2);
		int j = round(at_pos.x[idx]/sampling) + floor(xdim/2);
		//std::cout << k << " " << i << " " << j << "resi " << resi << std::endl;

		// Covalent Radius of the atom
		double covRad = at_pos.atomCovRad[idx];

		// Thermal displacement
		double bfactorRad = at_pos.b[idx];

		// Storing the bfactor per residue to compute the median
		bfactor_to_estimate.push_back(bfactorRad);

		// Updating the mean of bfactor
		bfactor_mean += bfactorRad;

		// Total Displacement in voxels
		int totRad = 3;//round( (covRad + bfactorRad)/sampling );

		int dim = totRad*totRad;


//		std::cout << "--------------------------------------" << std::endl;
                /*
		for (size_t kk = 0; kk<totRad; ++kk)
		{
			size_t kk2 = kk * kk;
			for (size_t jj = 0; jj<totRad; ++jj)
			{
				size_t jj2kk2 = jj * jj + kk2;
				for (size_t ii = 0; ii<totRad; ++ii)
				{
					size_t dist2 = (ii)*(ii) + (jj)*(jj) + (kk)*(kk);
					if (dist2 <= dim)
					{
						A3D_ELEM(mask, k-kk, i-ii, j-jj) = 1;
						A3D_ELEM(mask, k-kk, i-ii, j+jj) = 1;
						A3D_ELEM(mask, k-kk, i+ii, j-jj) = 1;
						A3D_ELEM(mask, k-kk, i+ii, j+jj) = 1;
						A3D_ELEM(mask, k+kk, i-ii, j-jj) = 1;
						A3D_ELEM(mask, k+kk, i-ii, j+jj) = 1;
						A3D_ELEM(mask, k+kk, i+ii, j-jj) = 1;
						A3D_ELEM(mask, k+kk, i+ii, j+jj) = 1;

						double &z1 = A3D_ELEM(resvol, k-kk, i-ii, j-jj);
						resolution_to_estimate.push_back(z1);
						resolution_mean += z1;
						N_elems++;

						double &z2 = A3D_ELEM(resvol, k-kk, i-ii, j+jj);
						resolution_to_estimate.push_back(z2);
						resolution_mean += z2;
						N_elems++;

						double &z3 = A3D_ELEM(resvol, k-kk, i+ii, j-jj);
						resolution_to_estimate.push_back(z3);
						resolution_mean += z3;
						N_elems++;

						double &z4 = A3D_ELEM(resvol, k-kk, i+ii, j+jj);
						resolution_to_estimate.push_back(z4);
						resolution_mean += z4;
						N_elems++;

						double &z5 = A3D_ELEM(resvol, k+kk, i-ii, j-jj);
						resolution_to_estimate.push_back(z5);
						resolution_mean += z5;

						double &z6 = A3D_ELEM(resvol, k+kk, i-ii, j+jj);
						resolution_to_estimate.push_back(z6);
						resolution_mean += z6;
						N_elems++;

						double &z7 = A3D_ELEM(resvol, k+kk, i+ii, j-jj);
						resolution_to_estimate.push_back(z7);
						resolution_mean += z7;
						N_elems++;

						double &z8 = A3D_ELEM(resvol, k+kk, i+ii, j+jj);
						resolution_to_estimate.push_back(z8);
						resolution_mean += z8;
						N_elems++;
					}
				}
			}
		}
                */
		for (size_t kk = 0; kk<totRad; ++kk)
		{
			size_t kk2 = kk * kk;
			for (size_t jj = 0; jj<totRad; ++jj)
			{
				size_t jj2kk2 = jj * jj + kk2;
				for (size_t ii = 0; ii<totRad; ++ii)
				{
					size_t dist2 = (ii)*(ii) + (jj)*(jj) + (kk)*(kk);
					if (dist2 <= dim)
					{
						A3D_ELEM(mask, k-kk, i-ii, j-jj) = 1;
						A3D_ELEM(mask, k-kk, i-ii, j+jj) = 1;
						A3D_ELEM(mask, k-kk, i+ii, j-jj) = 1;
						A3D_ELEM(mask, k-kk, i+ii, j+jj) = 1;
						A3D_ELEM(mask, k+kk, i-ii, j-jj) = 1;
						A3D_ELEM(mask, k+kk, i-ii, j+jj) = 1;
						A3D_ELEM(mask, k+kk, i+ii, j-jj) = 1;
						A3D_ELEM(mask, k+kk, i+ii, j+jj) = 1;

						double &z1 = A3D_ELEM(resvol, k-kk, i-ii, j-jj);
						resolution_to_estimate.push_back(z1);
						resolution_mean += z1;
						N_elems++;

						double &z2 = A3D_ELEM(resvol, k-kk, i-ii, j+jj);
						resolution_to_estimate.push_back(z2);
						resolution_mean += z2;
						N_elems++;

						double &z3 = A3D_ELEM(resvol, k-kk, i+ii, j-jj);
						resolution_to_estimate.push_back(z3);
						resolution_mean += z3;
						N_elems++;

						double &z4 = A3D_ELEM(resvol, k-kk, i+ii, j+jj);
						resolution_to_estimate.push_back(z4);
						resolution_mean += z4;
						N_elems++;

						double &z5 = A3D_ELEM(resvol, k+kk, i-ii, j-jj);
						resolution_to_estimate.push_back(z5);
						resolution_mean += z5;

						double &z6 = A3D_ELEM(resvol, k+kk, i-ii, j+jj);
						resolution_to_estimate.push_back(z6);
						resolution_mean += z6;
						N_elems++;

						double &z7 = A3D_ELEM(resvol, k+kk, i+ii, j-jj);
						resolution_to_estimate.push_back(z7);
						resolution_mean += z7;
						N_elems++;

						double &z8 = A3D_ELEM(resvol, k+kk, i+ii, j+jj);
						resolution_to_estimate.push_back(z8);
						resolution_mean += z8;
						N_elems++;
					}
				}
			}
		}


	}
	std::cout << "................................" << std::endl;

	std::vector<double> smoothedResolution(ma_c.size());
	smoothedResolution[0]=(ma_c[0]+ma_l[1])/(0.3+0.4); //0.3 left weight,, 0.4 central weight

	for (size_t i=1; i<(ma_c.size()-1); ++i)
	{
		smoothedResolution[i] = ma_l[i-1] + ma_c[i] + ma_l[i+1];
	}
	smoothedResolution[ma_c.size()-1] = (ma_c[ma_c.size()-1]+ma_l[ma_c.size()-2])/(0.3+0.4);

	std::vector<double> residuesToChimera_aux(at_pos.residue[idx_residue[last_index]]);

	MetaData mdSmooth;
	size_t objsmth;
	for (size_t nn = 0; nn<resolution_per_residue.size(); ++nn)
	{
		double bf, lr;
		int resnumber;
//		md.getValue(MDL_BFACTOR, bf, __iter.objId);
//		md.getValue(MDL_RESIDUE, resnumber, __iter.objId);
//		md.getValue(MDL_RESOLUTION_LOCAL_RESIDUE, lr, __iter.objId);
		lr = resolution_per_residue[nn];
		bf = bfactor_per_residue[nn];
		resnumber = resNumberList[nn];

		int idx_resnumber = (int) resnumber;

		lr = smoothedResolution[nn];
		residuesToChimera_aux[idx_resnumber-1] = lr;

		objsmth = md.addObject();
		md.setValue(MDL_BFACTOR, bf, objsmth);
		md.setValue(MDL_RESIDUE, resnumber, objsmth);
		md.setValue(MDL_RESOLUTION_LOCAL_RESIDUE, lr, objsmth);
	}

	FileName fn;

	fn = fnOut + "/bfactor_resolution.xmd";
	md.write(fn);

//	mdSmooth.write("smooth.xmd");
    residuesToChimera = residuesToChimera_aux;
	Image<int> imMask;
	imMask() = mask;
	imMask.write("mascara.mrc");

}


template <typename T>
void ProgResBFactor::maskFromPDBData(struct pdbdata &coord, MultidimArray<T> &mask)
{
	MultidimArray<double> resVol;
	Image<double> resVolImg;
	resVolImg.read(fn_locres);
	resVol = resVolImg();

	mask.resizeNoCopy(resVol);
	mask.initZeros();

	size_t xdim, ydim, zdim, ndim;
	resVol.getDimensions(xdim, ydim, zdim, ndim);

	std::cout << xdim << " " << ydim << " " << ndim << std::endl;
}


void ProgResBFactor::generateOutputPDB(std::vector<double> &residuesToChimera)
{
	//Open the pdb file
	std::ifstream f2parse;
	f2parse.open(fn_pdb.c_str());

	std::ofstream pdbToChimera;
	pdbToChimera.open(fnOut+"/chimeraPDB.pdb");

	int last_resi = 0;

	while (!f2parse.eof())
	{
		std::string line;
		getline(f2parse, line);

//		// The type of record (line) is defined in the first 6 characters of the pdb
		std::string typeOfline = line.substr(0,4);

		if ( (typeOfline == "ATOM") || (typeOfline == "HETA"))
		{

			std::string lineInit = line.substr(0,61);
			std::string lineEnd = line.substr(66, line.length()-1);

			int resi = (int) textToFloat(line.substr(23,5));
			std::string lineMiddle;
			int digitNumber = 5;

			std::stringstream ss;
			std::string auxstr;
			auxstr = std::to_string(residuesToChimera[resi-1]);

			auxstr = auxstr.substr(0, 5);
			ss << std::setfill('0') << std::setw(5)  << auxstr;
			lineMiddle = ss.str();

			std::string linePDB;
			linePDB = lineInit + lineMiddle + lineEnd;
			pdbToChimera << linePDB << std::endl;
		}
		else
		{
			pdbToChimera << line << std::endl;

		}
	}
	pdbToChimera.close();
}

//void ProgResBFactor::smoothingResidueOutput()
//{
//	std::cout << "starting smoothing" << std::endl;
//
//	MetaData md;
//
//	double mml, mmc, mmp;
//	int count = 0;
//
//	FOR_ALL_OBJECTS_IN_METADATA(md)
//	{
//		double bf, lr;
//		int resnumber;
//		md.getValue(MDL_BFACTOR, bf, __iter.objId);
//		md.getValue(MDL_RESIDUE, resnumber, __iter.objId);
//		md.getValue(MDL_RESOLUTION_LOCAL_RESIDUE, lr, __iter.objId);
//
//		mml = 0.3*lr;
//		mmc = 0.4*lr;
//
//		count++;
//
//	}
//
//	for (size_t i = 0; i<XXX; ++i)
//	{
//
//	}
//
//}


void ProgResBFactor::run()
{
	MultidimArray<int> mask;

	std::cout << "Start" << std::endl;
	analyzePDB();

	maskFromPDBData(at_pos, mask);

	std::vector<double> residuesToChimera;
	sweepByResidue(mask, residuesToChimera);
	
	generateOutputPDB(residuesToChimera);

//	Image<int> imgsave;
//	imgsave() = mask;
//	imgsave.write("mascara.mrc");

}
