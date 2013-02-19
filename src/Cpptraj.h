#ifndef INC_CPPTRAJ_H
#define INC_CPPTRAJ_H 
#include "TrajinList.h"
#include "TrajoutList.h"
#include "FrameList.h"
#include "TopologyList.h"
#include "DataSetList.h"
#include "DataFileList.h"
#include "ActionList.h"
#include "AnalysisList.h"
// Class: Cpptraj:
/// Hold state information.
/** This is the main class for cpptraj. It holds all data and controls the 
 *  overall flow of the program. It exists in main.cpp.
 */
class Cpptraj {
  public:
    enum Mode { C_OK = 0, C_ERR, C_QUIT, C_INTERACTIVE };
    Cpptraj();
    Mode Interactive();
    Mode ProcessInput(std::string const&);
    Mode ProcessCmdLineArgs(int,char**);
    int Run();
  private:
    static void Usage(const char*);
    static void Help_Help();
    static void Help_List();
    static void Help_Debug();
    static void Help_Clear();
    static void Help_ActiveRef();
    static void Help_Create_DataFile();
    static void Help_Precision();
    static void Help_SelectDS();
    static void Help_Trajin();
    static void Help_Ensemble();
    static void Help_Trajout();
    static void Help_Reference();
    static void Help_Parm();
    static void Help_ParmInfo();
    static void Help_ParmWrite();
    static void Help_ParmStrip();
    static void Help_ParmBox();
    static void Help_Solvent();
    static void Help_BondInfo();
    static void Help_ResInfo();
    static void Help_MolInfo();
    static void Help_CrdAction();
    static void Help_CrdOut();
    static void Help_RunAnalysis();

    /*class CommandObject {
      public:
        typedef int (Cpptraj::*FxnPtrType)(ArgList&,int);
        CommandObject() : type_(0), command_(0), fxn_(0) {}
        CommandObject(int t,const char* c,FxnPtrType f) : type_(t), command_(c), fxn_(f) {}
        CommandObject(const CommandObject& rhs) :
          type_(rhs.type_), command_(rhs.command_), fxn_(rhs.fxn_) {}
        DispatchObject::DispatchType Type() const { return type_;    }
        const char* Command()               const { return command_; }
        FxnPtrType Fxn()                    const { return fxn_;     }
        int Idx()                           const { return idx_;     }
      private:
        DispatchObject::DispatchType type_;
        const char* command_;
        FxnPtrType fxn_;
        int idx_;
    };
    std::vector<CommandObject> Cmds_;*/
    typedef int (Cpptraj::*FxnPtrType)(ArgList&,int);
    struct CommandObject {
      DispatchObject::DispatchType type_;
      const char* command_;
      DispatchObject::DispatchHelpType help_;
      FxnPtrType fxn_;
      int idx_;
    };
    static const CommandObject Cmds_[];

    int GeneralCmd(ArgList&,int);
    int Help(ArgList&,int);
    int List(ArgList&,int);
    int Debug(ArgList&,int);
    int Clear(ArgList&,int);
    int Create_DataFile(ArgList&,int);
    int Precision(ArgList&,int);
    int ReadData(ArgList&,int);
    int SelectDS(ArgList&,int);
    int LoadParm(ArgList&,int);
    int ParmInfo(ArgList&,int);
    int ParmWrite(ArgList&,int);
    int ParmStrip(ArgList&,int);
    int ParmBox(ArgList&,int);
    int ParmSolvent(ArgList&,int);
    int Select(ArgList&,int);
    int CrdAction(ArgList&,int);
    int CrdOut(ArgList&,int);
    int CrdAnalyze(ArgList&,int);

    //static const DispatchObject::Token GeneralCmds[];
    //static const DispatchObject::Token CoordCmds[];
    //static const DispatchObject::Token ParmCmds[];
    //static const DispatchObject::Token Deprecated[];
    void ListAllCommands(const DispatchObject::Token*);
    DispatchObject::TokenPtr SearchTokenArray(const DispatchObject::Token*, ArgList const&);
    DispatchObject::TokenPtr SearchToken(ArgList&);

    Mode Dispatch(std::string const&);     ///< Function that decides where to send commands

    /// List of parameter files 
    TopologyList parmFileList;
    /// List of input trajectory files
    TrajinList trajinList;
    /// List of reference coordinate files
    FrameList refFrames; 

    typedef std::vector<ArgList> ArgsArray;
    /// Array of trajout args for setting up ensemble trajout.
    ArgsArray trajoutArgs_;
    /// Array of action args for setting up ensemble actions.
    ArgsArray actionArgs_;

    /// List of output trajectory files 
    TrajoutList trajoutList;
    /// List of analyses to be performed on datasets
    AnalysisList analysisList;
    /// List of generated data sets
    DataSetList DSL;
    /// List of actions to be performed each frame
    ActionList actionList;    
    /// List of datafiles that data sets will be written to
    DataFileList DFL;
    /// The debug level
    int debug_;
    /// If true the progress of reading input trajectories will be shown
    bool showProgress_;
    /// If true cpptraj will exit if errors are encountered instead of trying to continue
    bool exitOnError_;
    /// Number of times the Run routine has been called.
    int nrun_;
    /// Log file for interactive mode
    CpptrajFile logfile_;

    int RunEnsemble();
    int RunNormal();
};
#endif
