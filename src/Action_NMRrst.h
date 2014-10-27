#ifndef INC_ACTION_NMRRST_H
#define INC_ACTION_NMRRST_H
#include "Action.h"
#include "ImagedAction.h"
#include "BufferedLine.h"
// Class: Action_NMRrst
class Action_NMRrst: public Action {
  public:
    Action_NMRrst();
    static DispatchObject* Alloc() { return (DispatchObject*)new Action_NMRrst(); }
    static void Help();
  private:
    Action::RetType Init(ArgList&, TopologyList*, FrameList*, DataSetList*,
                          DataFileList*, int);
    Action::RetType Setup(Topology*, Topology**);
    Action::RetType DoAction(int, Frame*, Frame**);
    void Print();

    int ReadNmrRestraints( std::string const& );
    int ReadXplor( BufferedLine& );
    int ReadAmber( BufferedLine& );
    /// Type to hold NOE data.
    struct noeDataType {
      int resNum1_;     ///< First residue number.
      int resNum2_;     ///< Second residue number.
      std::string aName1_; ///< First atom name.
      std::string aName2_; ///< Second atom name.
      AtomMask dMask1_; ///< First mask for distance pair.
      AtomMask dMask2_; ///< Second mask for distance pair.
      double bound_;    ///< Lower bound.
      double boundh_;   ///< Upper bound.
      double rexp_;     ///< Expected distance
      DataSet* dist_;   ///< Distance DataSet.
      bool active_;     ///< True if NOE was properly set up.
    };
    typedef std::vector<noeDataType> noeArray;
    noeArray NOEs_;

    typedef std::vector<int> Iarray;

    /// Potential NOE site.
    class Site {
      public:
        Site() : resNum_(-1) {}
        Site(int r, Iarray const& i) : resNum_(r), indices_(i), shortestCount_(i.size(), 0) {}
        int ResNum()                   const { return resNum_;           }
        unsigned int Nindices()        const { return indices_.size();   }
        int Idx(unsigned int i)        const { return indices_[i];       }
        int Count(unsigned int i)      const { return shortestCount_[i]; }
        void Increment(int c)                { ++shortestCount_[c];      }
//        int TotalCount()               const {
//          int c = 0;
//          for (Iarray::const_iterator it = shortestCount_.begin(); it != shortestCount_.end(); ++it)
//            c += *it;
//          return c;
//        };
      private:
        int resNum_; ///< Site residue number.
        Iarray indices_; ///< Site atom indices.
        Iarray shortestCount_; ///< # times atom was part of shortest distance.
    };
    typedef std::vector<Site> SiteArray;
    SiteArray potentialSites_;

    /// Used to map NOEs to unique values, res1 always < res2. 
    typedef std::pair<int,int> Ptype;

    /// NOE between two sites.
    class NOEtype {
      public:
        NOEtype() : dist2_(0), belowCut_(false) {}
        NOEtype(Site const& s1, Site const& s2, DataSet* d) :
          site1_(s1), site2_(s2), dist2_(d), belowCut_(false) {}
        Site const& Site1()    const { return site1_;    }
        Site const& Site2()    const { return site2_;    }
        bool CutoffSatisfied() const { return belowCut_; }
        DataSet* Data()              { return dist2_;    }
        void ResetData()             { dist2_ = 0;       }
        void UpdateNOE(int i, double d, unsigned int c1, unsigned int c2, bool satisfyCut) {
          float fval = (float)d;
          dist2_->Add(i, &fval);
          site1_.Increment( c1 );
          site2_.Increment( c2 );
          if (satisfyCut) belowCut_ = true;
        }
      private:
        Site site1_; ///< First site, lower resNum
        Site site2_; ///< Second site, higher resNum
        DataSet* dist2_; ///< Distance^2 data.
        bool belowCut_; ///< If true, cutoff was satisfied at least once.
    };
    typedef std::vector<NOEtype> NOEtypeArray;
    NOEtypeArray noeArray_;
//    typedef std::map<Ptype, NOEtype> NOEmap;
//    NOEmap FoundNOEs_;

    static void PrintFoundNOE(NOEtype const&);
    
    ImagedAction Image_;
    std::string setname_;
    DataSetList* masterDSL_; // TODO: Replace these with new DataSet type
    size_t numNoePairs_;
    double max_cut2_; ///< Min cutoff^2 for NOE to be present.
//    double present_fraction_; ///< NOEs present less than this will be removed.
    int resOffset_;
    int debug_;
    int nframes_; ///< Total # of frames.
    bool useMass_;
    bool findNOEs_;
};
#endif
