#include <cmath> // log, ldexp
#include "Analysis_Timecorr.h"
#include "CpptrajStdio.h"
#include "Constants.h" // PI

// Definition of Fortran subroutines called from this class
extern "C" {
  // pubfft.F90
  void cffti_(int&, double*);
  void cfftf_(int&, double*, double*);
  void cfftb_(int&, double*, double*);
}

// CONSTRUCTOR
Analysis_Timecorr::Analysis_Timecorr() :
  mode_(M_UNKNOWN),
  type_(T_UNKNOWN),
  dplr_(false),
  norm_(false),
  drct_(false),
  relax_(false),
  tstep_(1),
  tcorr_(10000),
  distnh_(1.02),
  freq_(-1),
  ndata_(0),
  table_(0),
  data1_(0),
  data2_(0),
  cf_(0),
  cfinf_(0),
  p2cf_(0),
  rcf_(0),
  cf_cjt_(0),
  taum_(0),
  vinfo1_(0),
  vinfo2_(0)
{}

Analysis_Timecorr::~Analysis_Timecorr() {
  if (cf_!=0) delete[] cf_;
  if (cfinf_!=0) delete[] cfinf_;
  if (p2cf_!=0) delete[] p2cf_;
  if (rcf_!=0) delete[] rcf_;
  if (data1_!=0) delete[] data1_;
  if (data2_!=0) delete[] data2_;
  if (table_!=0) delete[] table_;
  if (cf_cjt_!=0) delete[] cf_cjt_;
  if (taum_!=0) delete[] taum_;
}

// Analysis_Timecorr::Setup()
/** analyze timecorr vec1 <vecname1> [vec2 <vecname2>]
  *                  [relax] [freq <hz>] [NHdist <distnh>]
  *                  tstep <tstep> tcorr <tcorr> out <filename>
  */
int Analysis_Timecorr::Setup(DataSetList* DSLin) {
  // Ensure first 2 args (should be 'analyze' 'timecorr') are marked.
  analyzeArgs_.MarkArg(0);
  analyzeArgs_.MarkArg(1);
  // Get Vectors
  DSLin->VectorBegin();
  char* vec1 = analyzeArgs_.getKeyString("vec1",NULL);
  if (vec1==NULL) {
    mprinterr("Error: analyze timecorr: no vec1 given, ignoring command\n");
    return 1;
  }
  while ( (vinfo1_ = (VectorType*)DSLin->NextVector()) != 0 ) {
    if ( vinfo1_->Name().compare( vec1 ) == 0 ) break;
  }
  if (vinfo1_==0) {
    mprinterr("Error: analyze timecorr: no vector with name %s found.\n", vec1);
    return 1;
  }
  char* vec2 = analyzeArgs_.getKeyString("vec2",NULL);
  if (vec2!=NULL) {
    while ( (vinfo2_ = (VectorType*)DSLin->NextVector()) != 0 ) {
      if ( vinfo2_->Name().compare( vec2 ) == 0 ) break;
    }
    if (vinfo2_==0) {
      mprinterr("Error: analyze timecorr: no vector with name %s found.\n", vec2);
      return 1;
    }
  }

  // Determine mode and type
  // CORRIRED
  if (vinfo1_->Mode() == VectorType::VECTOR_CORRIRED) {
    mode_ = AUTO;
    type_ = IRED;
    if (vinfo2_!=0) {
      mprintf("Warning: analyze timecorr: only calculating IRED corr for vec1, ignoring vec2\n");
      vinfo2_ = 0;
    }
  // CORR, CORRPLANE
  } else if ( vinfo1_->Mode() == VectorType::VECTOR_CORR ||
              vinfo1_->Mode() == VectorType::VECTOR_CORRPLANE)
  {
    type_ = NORMAL;
    if (vinfo2_ == 0)
      mode_ = AUTO;
    else {
      // NOTE: Vector frame count is not yet initialized. Check in analysis.
      if ( (vinfo2_->Mode() != VectorType::VECTOR_CORR &&
            vinfo2_->Mode() != VectorType::VECTOR_CORRPLANE) ||
           //vinfo1_->Frame() != vinfo2_->Frame() ||
           vinfo1_->Order() != vinfo2_->Order() )
      {
        mprinterr("Error: analyze timecorr: different attributes for vec1 and vec2\n");
        return 1;
      }
      mode_ = CROSS;
    }
  // EVERYTHING ELSE
  } else {
    mprinterr("Error: analyze timecorr: wrong vector type given.\n");
    return 1;
  }
  // Sanity check
  if (mode_ == M_UNKNOWN || type_ == T_UNKNOWN) {
    mprinterr("Internal Error: analyze timecorr: Unknown mode or type.\n");
    return 1;
  }

  // Get tstep, tcorr, filename
  tstep_ = analyzeArgs_.getKeyDouble("tstep", 1.0);
  tcorr_ = analyzeArgs_.getKeyDouble("tcorr", 10000.0);
  noeFilename_ = analyzeArgs_.GetStringKey("noefile");
  filename_ = analyzeArgs_.GetStringKey("out");
  if (filename_.empty()) {
    mprinterr("Error: analyze timecorr: No outfile given ('out <filename>').\n");
    return 1;
  }

  // Get dplr, norm, drct, relax
  dplr_ = analyzeArgs_.hasKey("dplr");
  norm_ = analyzeArgs_.hasKey("norm");
  drct_ = analyzeArgs_.hasKey("drct");
  relax_ = analyzeArgs_.hasKey("relax");

  // Relax parameters
  if (relax_) {
    // Get freq, NH distance
    freq_ = analyzeArgs_.getKeyDouble("freq", -1.0);
    if (freq_ == -1.0) {
      mprinterr("Error: analyze timecorr: No frequency for calculation of relaxation\n");
      mprinterr("       parameters given ('freq <frequency>').\n");
      return 1;
    }
    // 1.02 * 10**(-10) in Angstroms
    distnh_ = analyzeArgs_.getKeyDouble("NHdist", 1.02);
  }

  // Print Status
  mprintf("    ANALYZE TIMECORR: Calculating");
  if (mode_ == AUTO)
    mprintf(" auto-correlation function of vector %s", vinfo1_->c_str());
  else if (mode_ == CROSS)
    mprintf(" cross-correlation function of vectors %s and %s", vinfo1_->c_str(),
            vinfo2_->c_str());
  mprintf("\n\t\tCorrelation time %lf, time step %lf\n", tcorr_, tstep_);
  mprintf("\t\tCorr. func. are");
  if (dplr_)
    mprintf(" for dipolar interactions and");
  if (norm_)
    mprintf(" normalized.\n");
  else
    mprintf(" not normalized.\n");
  mprintf("\t\tCorr. func. are calculated using the");
  if (drct_)
    mprintf(" direct approach.\n");
  else
    mprintf(" FFT approach.\n");
  if (relax_) {
    mprintf("\t\tTauM, relaxation rates, and NOEs are calculated using the iRED\n");
    mprintf("\t\t  approach using an NH distance of %lf and a frequency of %lf\n",
            distnh_, freq_);
  }
  if (!noeFilename_.empty())
    mprintf("\t\tNOEs and relaxation rates will be written to %s\n",
            noeFilename_.c_str());
  mprintf("\t\tResults are written to %s\n", filename_.c_str());

  return 0;
}

// TODO: Make static functions below part of the class

/** Calculates correlation functions using the "direct" approach
  * (s. Comp. Sim. of Liquids, p.185)
  * - the result is not yet normalized by (no_of_discrete_data - t)**-1 (!)
  */
static void corfdir(int ndata, double *data1, double *data2, int nsteps, double *dtmp)
{
  int i, j;
  int ind1, ind2;
  double dsum, dsumi;
  int ndata2 = ndata / 2; // TODO: Pass in

  if (data2 == NULL) {
    for(i = 0; i < ndata2; i++){
      dsum = 0.0;
      for(j = i; j < ndata2; j++){
        ind1 = 2 * j;
        ind2 = 2 * (j-i);
        dsum += data1[ind1] * data1[ind2] + data1[ind1+1] * data1[ind2+1];
      }
      if(i < nsteps){
        ind1 = 2 * i;
        dtmp[ind1  ] = dsum;
        dtmp[ind1+1] = 0.0;
      }
      else{
        break;
      }
    }
  } else {
    for(i = 0; i < ndata2; i++){
      dsum = 0.0;
      dsumi = 0.0;
      for(j = i; j < ndata2; j++){
        ind1 = 2 * j;
        ind2 = 2 * (j-i);
        dsum  += data2[ind1] * data1[ind2  ] + data2[ind1+1] * data1[ind2+1];
        dsumi += data2[ind1] * data1[ind2+1] - data2[ind1+1] * data1[ind2  ];
      }
      if(i < nsteps){
        ind1 = 2 * i;
        dtmp[ind1  ] = dsum;
        dtmp[ind1+1] = dsumi;
      }
      else{
        break;
      }
    }
  }

  for(i = 0; i < nsteps; i++){
    ind1 = 2 * i;
    data1[ind1  ] = dtmp[ind1  ];
    data1[ind1+1] = dtmp[ind1+1];
  }
}

/** Calculates correlation functions using the Wiener-Khinchin-Theorem
  * (s. Comp. Sim. of Liquids, p. 188)
  *
  * - ndata is the length of the arrays data1/2
  * - data1/2 is a real array of complex numbers: 
  *   data1/2(1)=real1/2(1), data1/2(2)=img1/2(1), ...
  * - it is recommended that the discrete data is appended
  *     with as much zeros to avoid spurious correlations
  * - in addition, data MUST have the dimension of power of 2
  *     (pad the "real" data (plus zeros) with additional 
  *     zeros up to the next power of 2)
  * - the result is not yet normalized by (no_of_discrete_data - t)**-1 (!)
  */
static void corffft(int ndata, double* data1, double* data2, double* table)
{
  int i, ndata2;
  double dtmp;

  ndata2 = ndata / 2; // TODO: pass in

  // FFT data
  //ftf_(&ndata2, data1, table);
  if(data2 != NULL)
    cfftf_(ndata2, data2, table);

  // Calc square modulus (in case of cross-correlation: calc [F(data1)]' * F(data2),
  //   where [F(data1)]' is the complex conjugate of F(data1)).
  if (data2 == NULL) {
    for(i = 0; i < ndata; i+=2){
      data1[i  ] = data1[i  ] * data1[i  ] + data1[i+1] * data1[i+1];
      data1[i+1] = 0.0;
    }
  } else {
    for(i = 0; i < ndata; i+=2){
      dtmp       = data1[i  ] * data2[i  ] + data1[i+1] * data2[i+1];
      data1[i+1] = data1[i  ] * data2[i+1] - data2[i  ] * data1[i+1];
      data1[i  ] = dtmp;
    }
  }

  // Inverse FFT
  cfftb_(ndata2, data1, table);

  // Normalize with ndata/2 (since not done in inverse FFT routine)
  dtmp = 1.0 / ((double) (ndata2));
  for(i = 0; i < ndata; i++)
    data1[i] *= dtmp;
}

/** Calc spectral density (JACS 2002, 124, 4522; eq. A24) */
/*static double calc_spectral_density(int nevec, int nelem, double *eigval, 
                                    double *vout, double *taum, int i, 
                                    double omega)
{
  double J = 0.0;
  for(int j = 0 ; j < nevec; j++){
    J += (eigval[j] * (vout[j * nelem + i] * vout[j * nelem + i])) * 2.0 * taum[j] /
         ( 1.0 + omega*omega * taum[j]*taum[j] ); //check order XXXX
  }                                                                                                  
  return J;
}*/

// Analysis_Timecorr::Analyze()
int Analysis_Timecorr::Analyze() {
  // If 2 vectors, ensure they have the same # of frames
  if (vinfo2_!=0) {
    if (vinfo1_->Frame() != vinfo2_->Frame()) {
      mprinterr("Error: # Frames in vec %s (%i) != # Frames in vec %s (%i)\n",
                vinfo1_->c_str(), vinfo1_->Frame(), vinfo2_->c_str(), vinfo2_->Frame());
      return 1;
    }
  }

  // Determine sizes
  int frame = vinfo1_->Frame();
  int time = (int)(tcorr_ / tstep_) + 1;
  // nsteps
  int nsteps = 0;
  if (time > frame)
    nsteps = frame;
  else
    nsteps = time;
  // ndata
  int ndata = 0;
  if (drct_)
    ndata = 2 * frame;
  else
    ndata = ldexp( 1.0, (int)(log((double)(4 * frame - 1)) / log(2.0)) + 1);
  int ndata2 = ndata / 2;
  int mtot = 2 * vinfo1_->Order() + 1;

  // Allocate common memory and initialize arrays
  data1_ = new double[ ndata ];
  if (mode_ == CROSS)
    data2_ = new double[ ndata ];
  if (drct_)
    table_ = new double[ 2 * nsteps ];
  else
    table_ = new double[ 2 * ndata + 15 ];

  // Initialize FFT
  if (!drct_)
    cffti_(ndata2, table_);

  // ---------- IRED CALCULATION -----------------
  if (type_ == IRED) {
    double* cftmp1 = vinfo1_->Cftmp();
    // Initialize memory
    ModesInfo* modinfo = vinfo1_->ModeInfo();
    int nvect = modinfo->Nvect();
    int nvectelem = modinfo->NvectElem();
    double* eigval = modinfo->Freq();
    double* vout = modinfo->Evec();
    cf_ = new double[ nvect * nsteps ];
    cf_cjt_ = new double[ nvect * nsteps ];
    for (int i = 0; i < nvect*nsteps; ++i) {
      cf_[i] = 0;
      cf_cjt_[i] = 0;
    }
    cfinf_ = new double[ nvect ];
    for (int i = 0; i < nvect; ++i)
      cfinf_[i] = 0;
    if (relax_) {
      taum_ = new double[ nvect ];
      for (int i = 0; i < nvect; ++i)
        taum_[i] = 0;
    }

    // Loop over all eigenvectors
    for (int i = 0; i < nvect; ++i) {
      int idx1 = nsteps * i;
      // Loop over all m = -L, ...., L
      for (int j = 0; j < mtot; ++j) {
        int idx2 = nvect * j;
        // Loop over all snapshots
        double cfinfavgreal = 0;
        double cfinfavgimg = 0;
        for (int k = 0; k < frame; ++k) {
          int idx3 = 2 * (nvect * mtot * k + idx2 + i);
          data1_[2*k  ] = cftmp1[idx3  ];
          cfinfavgreal += cftmp1[idx3  ];
          data1_[2*k+1] = cftmp1[idx3+1];
          cfinfavgimg  += cftmp1[idx3+1];
        }
        cfinfavgreal /= frame;
        cfinfavgimg  /= frame;
        // Calc plateau value of correlation function (= C(m,t->T) in Bruschweiler paper (A20))
        cfinf_[i] += (cfinfavgreal * cfinfavgreal) + (cfinfavgimg * cfinfavgimg);
        if (drct_) {
          // Calc correlation function (= C(m,l,t) in Bruschweiler paper) using direct approach
          corfdir(ndata, data1_, NULL, nsteps, table_);
        } else {
          // Pad with zero's at the end
          for (int k = 2*frame; k < ndata; ++k)
            data1_[k] = 0;
          // Calc correlation function (= C(m,l,t) in Bruschweiler paper) using FFT
          corffft(ndata, data1_, NULL, table_);
        }
        // Sum into cf (= C(m,t) in Bruschweiler paper)
        for (int k = 0; k < nsteps; ++k)
          cf_[idx1 + k] += data1_[2 * k];
      }
    }
    // Calculate correlation function for each vector:
    // Cj(t) according to eq. A23 in Prompers & Brüschweiler, JACS  124, 4522, 2002; 
    // added by A.N. Koller & H. Gohlke
    for (int i = 0; i < nvect; ++i) {
      for (int k = 0; k < nsteps; ++k) {
        double sum = 0;
        for (int j = 0; j < nvect; ++j) {
          sum += (eigval[j] * (vout[j * nvectelem + i] * vout[j * nvectelem + i])) *
                 (cf_[nsteps * j + k] / (frame - k));
        }
        cf_cjt_[nsteps * i + k] = sum;
      }
    }

    if (relax_) {
      // TauM calculation: eq. A19 from Prompers & Brüschweiler, JACS  124, 4522, 2002 is used.
      // Values are calculated from normalized correlation functions (Cm(t)s).
      // Added by Alrun N. Koller & H. Gohlke
      // Conversion to picoseconds
      double deltat = tstep_ * 1.0E-12;
      if (nvectelem != nvect) {
        mprinterr("Error: analyze timecorr: Different number of eigenmodes (%i) and\n", nvect);
        mprinterr("       eigenmode elements (%i)\n", nvectelem);
        return 1;
      }
      // consider only first third of the correlation curve to avoid fitting errors
      double new_nsteps = nsteps / 3;
      for (int i = 0; i < nvect; ++i) {
        double integral = 0;
        double cfinfval = cfinf_[i] / cf_[nsteps * i] * frame;
        for ( int j = 0 ; j < new_nsteps; ++j ) {
            double cfk = cf_[nsteps * i + j] * frame / (cf_[nsteps * i] * (frame - j));
            double cfk1 = cf_[nsteps * i + j + 1 ] * frame / (cf_[nsteps * i] * (frame - (j+1)));
            double fa = cfk - cfinfval;
            double fb = cfk1 - cfinfval;
            integral += deltat * ( fa + fb ) * 0.5;
        }
        double taum_val = integral / ( 1.0 - cfinfval );
        taum_[i] = taum_val;
      }

      // Relaxation calculation. Added by Alrun N. Koller & H. Gohlke
      CpptrajFile noefile;
      int err = 0;
      if (noeFilename_.empty())
        err = noefile.SetupWrite(NULL, debug_);
      else
        err = noefile.SetupWrite(noeFilename_.c_str(), debug_);
      if (err != 0) {
        mprinterr("Error: analyze timecorr: Could not open NOE file for write.\n");
        return 1;
      }
      noefile.OpenFile();
      noefile.Printf("\n\t****************************************");
      noefile.Printf("\n\t- Calculated relaxation rates and NOEs -");
      noefile.Printf("\n\t****************************************\n\n");
      noefile.Printf("vector   %10s   %10s   %10s\n","R1","R2","NOE");
      // conversion from Angstrom to meter
      double rnh = distnh_ * 1.0E-10;
      // ---------- CONSTANTS ----------
      // in H m^-1; permeability
      const double mu_zero = 4.0 * PI * 1.0E-7;
      // in m^2 kg s1-1 ; Js ; Planck's constant
      const double ha = 6.626176 * 1.0E-34;
      // in rad s^-1 T^-1 ; gyromagnetic ratio; T = absolut temperature in K
      const double gamma_h = 2.6751987 * 1.0E8;
      // in rad s^-1 T^-1 ; gyromagnetic ratio
      const double gamma_n = -2.7126 * 1.0E7;
      const double csa = -170.0 * 1.0E-6;
      // conversion from MHz to rad s^-1
      //double spec_freq = freq_ * TWOPI * 1.0E6;
      // in T (Tesla)
      double b_zero = TWOPI * freq_ * 1.0E6 / gamma_h;
      // Next two both in rad s^-1
      const double lamfreqh = -1 * gamma_h * b_zero;
      const double lamfreqn = -1 * gamma_n * b_zero;
      double c2 = lamfreqn*lamfreqn * csa*csa;
      double d2 = (mu_zero * ha * gamma_n * gamma_h)/( 8.0 * (PI*PI) * (rnh*rnh*rnh));
      d2 = d2*d2; // fix from Junchao
      // -------------------------------
      // loop over all vector elements --> have only one element/vector here; nvectelem = nelem
      for (int i = 0; i < nvectelem; ++i) {
        // Eq. A1 in Prompers & Brüschweiler, JACS  124, 4522, 2002
        double R1 = d2 / 20.0 * (
              modinfo->calc_spectral_density( taum_, i, lamfreqh - lamfreqn ) 
              + 3.0 * modinfo->calc_spectral_density( taum_, i, lamfreqn ) 
              + 6.0 * modinfo->calc_spectral_density( taum_, i, lamfreqh + lamfreqn)
            ) + c2 / 15.0 * modinfo->calc_spectral_density( taum_, i, lamfreqn );
        // Eq. A2 in Prompers & Brüschweiler, JACS  124, 4522, 2002
        double R2 = d2 / 40.0 * ( 4.0 *
              modinfo->calc_spectral_density(  taum_, i, 0.0 ) 
              + modinfo->calc_spectral_density( taum_, i, lamfreqh - lamfreqn )
              + 3.0 * modinfo->calc_spectral_density( taum_, i, lamfreqn )
              + 6.0 * modinfo->calc_spectral_density( taum_, i, lamfreqh ) 
              + 6.0 * modinfo->calc_spectral_density( taum_, i, lamfreqh + lamfreqn)
            ) + c2 / 90.0 * ( 4.0 *
              modinfo->calc_spectral_density( taum_, i, 0.0 ) 
              + 3.0 * modinfo->calc_spectral_density( taum_, i, lamfreqn) );
        // Eq. A3 in Prompers & Brüschweiler, JACS  124, 4522, 2002
        double Tj = d2 / 20.0 * ( 6.0 *
              modinfo->calc_spectral_density( taum_, i, lamfreqh + lamfreqn ) 
              - modinfo->calc_spectral_density( taum_, i, lamfreqh + lamfreqn ) );

        double Noe = 1.0 + ( gamma_h * 1.0/gamma_n ) * ( 1.0 / R1 ) * Tj;
        noefile.Printf("%6i   %10.5f   %10.5f   %10.5f\n", i, R1, R2, Noe);
      }
      noefile.Printf("\n\n");
      noefile.CloseFile();
    } // END if (relax_)

  // ---------- NORMAL CALCULATION ---------------
  } else if (type_ == NORMAL) {
    // Initialize memory
    p2cf_ = new double[ nsteps ];
    for (int i = 0; i < nsteps; ++i)
      p2cf_[i] = 0;
    if (dplr_) {
      cf_ = new double[ nsteps ];
      rcf_ = new double[ nsteps ];
      for (int i = 0; i < nsteps; ++i) {
        cf_[i] = 0;
        rcf_[i] = 0;
      }
    }

  }

  return 0;
}
