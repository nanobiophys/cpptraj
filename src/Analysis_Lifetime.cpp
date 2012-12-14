#include "Analysis_Lifetime.h"
#include "CpptrajStdio.h"
#include "ProgressBar.h"

// CONSTRUCTOR
Analysis_Lifetime::Analysis_Lifetime() :
  windowSize_(0),
  cut_(0.5),
  averageonly_(false),
  cumulative_(false),
  deltaAvg_(false)
{}

void Analysis_Lifetime::Help() {
  mprintf("lifetime [out <filename>] <dsetarg0> [ <dsetarg1> ... ]\n");
  mprintf("         [window <windowsize> [name <setname>]] [averageonly]\n");
  mprintf("         [cumulative] [cut <cutoff>]\n");
}

Analysis::RetType Analysis_Lifetime::Setup(ArgList& analyzeArgs, DataSetList* datasetlist,
                            TopologyList* PFLin, int debugIn)
{
  // Get Keywords
  outfilename_ = analyzeArgs.GetStringKey("out");
  std::string setname_ = analyzeArgs.GetStringKey("name");
  windowSize_ = analyzeArgs.getKeyInt("window", -1);
  averageonly_ = analyzeArgs.hasKey("averageonly");
  cumulative_ = analyzeArgs.hasKey("cumulative");
  deltaAvg_ = analyzeArgs.hasKey("delta");
  cut_ = analyzeArgs.getKeyDouble("cut", 0.5);
  // Select datasets from remaining args
  ArgList dsetArgs = analyzeArgs.RemainingArgs();
  for (ArgList::const_iterator dsa = dsetArgs.begin(); dsa != dsetArgs.end(); ++dsa)
    inputDsets_ += datasetlist->GetMultipleSets( *dsa );
  if (inputDsets_.empty()) {
    mprinterr("Error: lifetime: No data sets selected.\n");
    return Analysis::ERR;
  }
  // Sort input datasets
  inputDsets_.sort();

  // Create output datasets
  if ( windowSize_ != -1) {
    if (setname_.empty()) 
      setname_ = datasetlist->GenerateDefaultName( "lifetime" );
    int didx = 0;
    for (DataSetList::const_iterator set = inputDsets_.begin(); set != inputDsets_.end(); ++set)
    {
      DataSet* outSet = datasetlist->AddSetIdx( DataSet::FLOAT, setname_, didx );
      if (outSet==0) {
        mprinterr("Error: lifetime: Could not allocate output set for %s\n", 
                  (*set)->Legend().c_str());
        return Analysis::ERR;
      }
      outSet->SetLegend( (*set)->Legend() );
      outputDsets_.push_back( outSet );
      if (!averageonly_) {
        // MAX
        // FIXME: CHeck for nullS
        outSet = datasetlist->AddSetIdxAspect( DataSet::INT, setname_, didx, "max" );
        outSet->SetLegend( (*set)->Legend() );
        maxDsets_.push_back( outSet );
        // AVG
        outSet = datasetlist->AddSetIdxAspect( DataSet::FLOAT, setname_, didx, "avg" );
        outSet->SetLegend( (*set)->Legend() );
        avgDsets_.push_back( outSet );
      }
      ++didx;
    }
  } else if (!outfilename_.empty()) {
    mprinterr("Error: Output file name specified but no window size given ('window <N>')\n");
    return Analysis::ERR;
  }

  if (!averageonly_)
    mprintf("    LIFETIME: Calculating average lifetime using a cutoff of %f", cut_);
  else
    mprintf("    LIFETIME: Calculating only averages");
  mprintf(" of data in %i sets\n", inputDsets_.size());
  if (debugIn > 0)
    inputDsets_.List();
  if (windowSize_ != -1) {
    mprintf("\tAverage of data over windows will be saved to sets named %s\n",
            setname_.c_str());
    mprintf("\tWindow size for averaging: %i\n", windowSize_);
    if (cumulative_)
      mprintf("\tCumulative averages will be saved.\n");
    if (deltaAvg_)
      mprintf("\tChange of average from previous average will be saved.\n");
    if (!outfilename_.empty()) {
      mprintf("\tOutfile: %s", outfilename_.c_str());
      if (!averageonly_)
        mprintf(", max.%s, avg.%s", outfilename_.c_str(), outfilename_.c_str());
      mprintf("\n");
    }
  }

  return Analysis::OK;
}

// Analysis_Lifetime::Analyze()
Analysis::RetType Analysis_Lifetime::Analyze() {
  float favg;
  int current = 0;
  ProgressBar progress( inputDsets_.size() );
  std::vector<DataSet*>::iterator outSet = outputDsets_.begin();
  std::vector<DataSet*>::iterator maxSet = maxDsets_.begin();
  std::vector<DataSet*>::iterator avgSet = avgDsets_.begin();
  for (DataSetList::const_iterator inSet = inputDsets_.begin(); 
                                   inSet != inputDsets_.end(); ++inSet)
  {
    //mprintf("\t\tCalculating lifetimes for set %s\n", (*inSet)->Legend().c_str());
    progress.Update( current++ );
    // Loop over all values in set.
    int setSize = (*inSet)->Size();
    double sum = 0.0;
    double previous_windowavg = 0.0;
    int windowcount = 0; // Used to trigger averaging
    int Ncount = 0;      // Used in averaging; if !cumulative, == windowcount
    int frame = 0;       // Frame to add data at.
    int currentLifetimeCount = 0; // # of frames value has been present this lifetime
    int maximumLifetimeCount = 0; // Max observed lifetime
    int Nlifetimes = 0;           // # of separate lifetimes observed
    int sumLifetimes = 0;         // sum of lifetimeCount for each lifetime observed
    for (int i = 0; i < setSize; ++i) {
      double dval = (*inSet)->Dval(i);
      //mprintf("\t\t\tValue[%i]= %.2f", i,dval);
      if (averageonly_) 
        // Average only
        sum += dval;
      else {
        // Lifetime calculation
        if ( dval > cut_ ) {
          // Value is present at time i
          ++sum;
          ++currentLifetimeCount;
          //mprintf(" present; sum=%i LC=%i\n", sum, currentLifetimeCount);
        } else {
          //mprintf(" not present");
          // Value is not present at time i
          if (currentLifetimeCount > 0) {
            if (currentLifetimeCount > maximumLifetimeCount)
              maximumLifetimeCount = currentLifetimeCount;
            sumLifetimes += currentLifetimeCount;
            ++Nlifetimes;
            //mprintf("; LC=%i maxLC=%i NL=%i\n", currentLifetimeCount, maximumLifetimeCount, Nlifetimes);
            currentLifetimeCount = 0;
          }
          //mprintf("\n");
        }
      }
      //sum += (*inSet)->Dval(i);
      ++Ncount;
      ++windowcount;
      if (windowcount == windowSize_) {
        double windowavg = sum / (double)Ncount;
        float fval = (float)(windowavg - previous_windowavg);
        if (deltaAvg_) previous_windowavg = windowavg;
        (*outSet)->Add( frame, &fval );
        if (!averageonly_) {
          // Store lifetime information for this window
          // Update current lifetime total
          if (currentLifetimeCount > 0) {
            if (currentLifetimeCount > maximumLifetimeCount)
              maximumLifetimeCount = currentLifetimeCount;
            sumLifetimes += currentLifetimeCount;
            ++Nlifetimes;
          }
          // If Nlifetimes is 0 then value was never present. 
          if (Nlifetimes == 0) 
            favg = 0.0;
          else
            favg = (float)sumLifetimes / (float)Nlifetimes;
          //mprintf("\t\t\t[%i]Max lifetime observed: %i frames\n", frame,maximumLifetimeCount);
          //mprintf("\t\t\t[%i]Avg lifetime: %f frames\n", frame, favg);
          (*maxSet)->Add( frame, &maximumLifetimeCount );
          (*avgSet)->Add( frame, &favg );
        }
        frame += windowcount;
        // Window counter is always reset
        windowcount = 0;
        if (!cumulative_) {
          // Reset average counters
          sum = 0;
          Ncount = 0;
          // Reset lifetime counters
          currentLifetimeCount = 0;
          maximumLifetimeCount = 0;
          Nlifetimes = 0;
          sumLifetimes = 0;
        }
      }
    }
    // Print lifetime information if no window
    if ( !averageonly_ && windowSize_ == -1 ) {
      // Update current lifetime total
      if (currentLifetimeCount > 0) {
        if (currentLifetimeCount > maximumLifetimeCount)
          maximumLifetimeCount = currentLifetimeCount;
        sumLifetimes += currentLifetimeCount;
        ++Nlifetimes;
      }
      // If Nlifetimes is 0 then value was never present. 
      if (Nlifetimes == 0) 
        favg = 0.0;
      else
        favg = (float)sumLifetimes / (float)Nlifetimes;
      mprintf("\t\t\tMax lifetime observed: %i frames\n", maximumLifetimeCount);
      //mprintf("\t\tSumLifeTimes=%i  Nlifetimes=%i\n",sumLifetimes,Nlifetimes);
      mprintf("\t\t\tAvg lifetime: %f frames\n", favg);
    }
    ++outSet;
    ++maxSet;
    ++avgSet;
  }
  return Analysis::OK;
}

void Analysis_Lifetime::PrintListToFile(DataFileList *dfl, std::vector<DataSet*>& list,
                                        std::string const& outname)
{
  DataFile *outfile = 0;
  if (outname.empty()) return;
  for (std::vector<DataSet*>::iterator set = list.begin(); set != list.end(); ++set)
  {
    outfile = dfl->AddSetToFile( outname, *set );
    if (outfile == 0) {
      mprinterr("Error adding set %s to file %s\n", (*set)->Legend().c_str(), outname.c_str());
      return;
    }
  }
  outfile->ProcessArgs("noemptyframes");
}

void Analysis_Lifetime::Print(DataFileList* datafilelist) {
  if (!outfilename_.empty()) {
    PrintListToFile(datafilelist, outputDsets_, outfilename_);
    if (!averageonly_) {
      PrintListToFile(datafilelist, maxDsets_, "max." + outfilename_);
      PrintListToFile(datafilelist, avgDsets_, "avg." + outfilename_);
    }
  }
}

