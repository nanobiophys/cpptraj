//DataFile
#include <cstdlib>
#include <cstring>
#include <cstdio> //sprintf for frame #
#include "DataFile.h"
#include "CpptrajStdio.h"

// CONSTRUCTOR
DataFile::DataFile() {
  xlabel=NULL;
  ylabel=NULL;
  noEmptyFrames=false;
  noXcolumn=false;
  filename=NULL;
  SetList=NULL;
  Nsets=0;
  filename=NULL;
  debug=0;
  isInverted=false;
  maxFrames = 0;

  xmin=1;
  xstep=1;
  ymin=1;
  ystep=1;
  useMap=false;
  printLabels=true;
}

// CONSTRUCTOR - Arg is datafile name
DataFile::DataFile(char *nameIn) {
  // Default xlabel value is Frame
  xlabel=(char*) malloc( 6 * sizeof(char));
  strcpy(xlabel,"Frame");
  // Default ylabel value is blank
  ylabel=(char*) malloc( sizeof(char));
  strcpy(ylabel,"");
  noEmptyFrames=false;
  noXcolumn=false;
  filename=NULL;
  SetList=NULL;
  Nsets=0;
  filename=(char*) malloc( (strlen(nameIn)+1) * sizeof(char));
  strcpy(filename,nameIn);
  debug=0;
  isInverted=false;
  maxFrames = 0;
  xmin=1;
  xstep=1;
  ymin=1;
  ystep=1;
  useMap=false;
  printLabels=true;
}

// DESTRUCTOR
DataFile::~DataFile() {
  if (filename!=NULL) free(filename);
  // Individual data sets are freed in DataSetList
  if (SetList!=NULL) free(SetList);
  if (xlabel!=NULL) free(xlabel);
  if (ylabel!=NULL) free(ylabel);
}

/* DataFile::SetDebug
 * Set DataFile debug level
 * NOTE: Pass in constructor?
 */
void DataFile::SetDebug(int debugIn) {
  if (debug==debugIn) return;
  debug=debugIn;
  if (debug>0)
    mprintf("DataFile %s DEBUG LEVEL SET TO %i\n",filename,debug);
}

/* DataFile::SetNoXcol
 * Turn printing of frame column off.
 */
void DataFile::SetNoXcol() {
  noXcolumn=true;
}

/* DataFile::SetNoEmptyFrames()
 */
void DataFile::SetNoEmptyFrames() {
  noEmptyFrames=true;
}

/* DataFile::SetXlabel()
 */
void DataFile::SetXlabel(char *labelIn) {
  if (labelIn==NULL) return;
  xlabel = (char*) realloc ( xlabel, (strlen(labelIn)+1) * sizeof(char) );
  strcpy(xlabel,labelIn);
}

/* DataFile::SetYlabel()
 */
void DataFile::SetYlabel(char *labelIn) {
  if (labelIn==NULL) return;
  ylabel = (char*) realloc ( ylabel, (strlen(labelIn)+1) * sizeof(char) );
  strcpy(ylabel,labelIn);
}

/* DataFile::SetInverted()
 */
void DataFile::SetInverted() {
  isInverted=true;
}

/* DataFile::SetCoordMinStep()
 * For use with certain types of output.
 */
void DataFile::SetCoordMinStep(double xminIn, double xstepIn, 
                               double yminIn, double ystepIn) {
  xmin = xminIn;
  xstep = xstepIn;
  ymin = yminIn;
  ystep = ystepIn;
}

/* DataFile::SetMap()
 * Gnuplot only currently. Turn off the cornerstocolor option, i.e.
 * gnuplot will be directed to generated a true interpolated contour
 * map.
 */
void DataFile::SetMap() {
  useMap=true;
}

/* DataFile::SetNoLabels()
 * Gnuplot only currently. Do not print y axis labels (dataset names). This
 * will automatically be turned off if the # labels > 30.
 */
void DataFile::SetNoLabels() {
  printLabels=false;
}

/* DataFile::SetPrecision()
 * Set precision of the specified dataset to width.precision. If '*' specified 
 * set for all datasets in file.
 */
void DataFile::SetPrecision(char *dsetName, int widthIn, int precisionIn) {
  int precision, dset;
  DataSet *Dset = NULL;

  if (dsetName==NULL) {
    mprintf("Error: SetPrecision must be called with dataset name or '*'.\n");
    return;
  }
  if (widthIn<1) {
    mprintf("Error: SetPrecision (%s): Cannot set width < 1.\n",filename);
    return;
  }
  precision=precisionIn;
  if (precisionIn<0) precision=0;
  // If <dsetName>=='*' specified set precision for all data sets
  if (dsetName[0]=='*') {
    mprintf("    Setting width.precision for all sets in %s to %i.%i\n",
            filename,widthIn,precision);
    for (dset=0; dset<Nsets; dset++)
      SetList[dset]->SetPrecision(widthIn,precision);

  // Otherwise find dataset <dsetName> and set precision
  } else {
    mprintf("    Setting width.precision for dataset %s to %i.%i\n",
            dsetName,widthIn,precision);
    for (dset=0; dset<Nsets; dset++) {
      if ( strcmp(SetList[dset]->Name(), dsetName)==0 ) {
        Dset=SetList[dset];
        break;
      }
    }
    if (Dset!=NULL)
      Dset->SetPrecision(widthIn,precision);
    else
      mprintf("Error: Dataset %s not found in datafile %s\n",dsetName,filename);
  }
}

/* DataFile::AddSet()
 * Add given set to this datafile
 */
int DataFile::AddSet(DataSet *D) {

  SetList = (DataSet**) realloc( SetList, (Nsets+1) * sizeof(DataSet*));
  SetList[Nsets]=D;
  Nsets++;
  return 0;
}

/* DataFile::NameIs()
 * Return 1 if datafile name matches nameIn
 */
int DataFile::NameIs(char *nameIn) {
  if (strcmp(nameIn, filename)==0) return 1;
  return 0;
}

/* DataFile::DataSetNames()
 * Print Dataset names to one line. If the number of datasets is very
 * large just print the number of data sets.
 */
void DataFile::DataSetNames() {
  int set;
  if (Nsets>10) {
    for (set=0; set < 4; set++)
      mprintf(" %s",SetList[set]->Name());
    mprintf(" ...");
    for (set=Nsets-4; set < Nsets; set++)
      mprintf(" %s",SetList[set]->Name()); 
    //mprintf("%i datasets.",Nsets);
  } else {
    for (set=0; set<Nsets; set++) {
      if (set>0) mprintf(",");
      mprintf("%s",SetList[set]->Name());
    }
  }
}

/* DataFile::Write()
 * Write datasets to file. Check that datasets actually contain data. 
 * Exit if no datasets in this datafile have been used.
 */
void DataFile::Write() {
  PtrajFile outfile;
  int set,nwrite;
  int maxSetFrames = 0;
  int currentMax = 0;
  int NnewList = 0;
  DataSet **newList;

  // Remove data sets that do not contain data from SetList.
  // NOTE: CheckSet also sets up the format string for the dataset.
  newList = (DataSet**) malloc( Nsets * sizeof(DataSet*) );
  nwrite=0;
  for (set=0; set<Nsets; set++) {
    if ( SetList[set]->CheckSet() ) {
      mprintf("Warning: DataFile %s: Set %s contains no data - skipping.\n",
              filename, SetList[set]->Name());
      //return;
    } else {
      newList[NnewList++] = SetList[set];
      // Determine what the maxium x value for this set is.
      // Should be last value added.
      maxSetFrames = SetList[set]->Xmax();
      if (maxSetFrames > currentMax) currentMax = maxSetFrames;
      nwrite++;
    }
  }
  // Reset SetList
  free(SetList);
  SetList = newList;
  Nsets = NnewList;
  // If all data sets are empty then no need to write
  if (nwrite==0) {
    mprintf("Warning: DataFile %s has no sets containing data - skipping.\n",filename);
    return;
  }
  // Since currentMax is the last frame, increment currentMax by 1 for use in for loops
  maxFrames = currentMax + 1;
  //mprintf("DEBUG: Max frames for %s is %i (maxFrames=%i)\n",filename,currentMax,maxFrames);

  if (outfile.SetupFile(filename,WRITE,UNKNOWN_FORMAT,UNKNOWN_TYPE,debug)) return;
  if (outfile.OpenFile()) return;

  // If format is unknown default to DATAFILE
  if (outfile.fileFormat == UNKNOWN_FORMAT)
    outfile.fileFormat = DATAFILE;

  switch (outfile.fileFormat) {
    case DATAFILE : 
      if (isInverted)
        this->WriteDataInverted(&outfile);
      else 
        this->WriteData(&outfile); 
      break;
    case XMGRACE  : 
      if (isInverted)
        this->WriteGraceInverted(&outfile);
      else 
        this->WriteGrace(&outfile); 
      break;
    case GNUPLOT  :
      if (isInverted)
        mprintf("Warning: Gnuplot format does not support invert; printing standard.\n");
      this->WriteGnuplot(&outfile);
      break;
    default       : mprintf("Error: Datafile %s: Unknown type.\n",filename);
  }

  outfile.CloseFile();
}

/* DataFile::WriteData()
 * Write datasets to file. Put each set in its own column. 
 */
void DataFile::WriteData(PtrajFile *outfile) {
  int set,frame,empty;
  char *ptr,*buffer;
  int lineSize = 0;
  bool firstSet = true;
  size_t dataFileSize = 0;

  buffer=NULL;
  // Calculate overall size of the datafile. 
  if (!noXcolumn) lineSize = 8;
  for (set=0; set<Nsets; set++)
    lineSize += SetList[set]->Width();
  // Add 1 to lineSize for newline
  lineSize++;
  // Datafile size is line size times maxFrames+1 lines
  dataFileSize = (size_t) maxFrames;
  dataFileSize++;
  dataFileSize *= (size_t) lineSize;
  // NOTE: Since we are manually writing the newlines to this file there is
  //       no need to allocate space for a NULL char. If something like 
  //       sprintf is ever used 1 more byte needs to be allocd for NULL.
  buffer = (char*) malloc (dataFileSize * sizeof(char));
  if (buffer==NULL) {
    mprinterr("Error: DataFile %s: Could not allocate buffer memory.\n",filename);
    return;
  }

  // Write header to buffer
  ptr = buffer;
  if (!noXcolumn) {
    // NOTE: Eventually use xlabel
    strncpy(ptr,"#Frame  ",8);
    ptr+=8;
  }
  for (set=0; set<Nsets; set++) {
    if (noXcolumn && firstSet) {
      ptr = SetList[set]->Name(ptr,true);
      firstSet=false;
    } else
      ptr = SetList[set]->Name(ptr,false);
  }
  ptr[0]='\n';
  ptr++;

  // Write Data to buffer
  for (frame=0; frame<maxFrames; frame++) {
    // If specified, run through every set in the frame and check if empty
    if (noEmptyFrames) {
      empty=0;
      for (set=0; set<Nsets; set++) {
        if ( SetList[set]->isEmpty(frame) ) {empty++; break;}
      } 
      if (empty!=0) continue;
    }
    // Output Frame
    // NOTE: For consistency with Ptraj start at frame 1
    if (!noXcolumn) {
      sprintf(ptr,"%8i",frame + OUTPUTFRAMESHIFT);
      ptr+=8;
    }
    for (set=0; set<Nsets; set++) {
      ptr = SetList[set]->Write(ptr,frame);
    }
    ptr[0]='\n';
    ptr++;
  }

  // If noEmptyFrames the actual size of data written may be less than
  // the size of the data allocated. Recalculate the actual size.
  if (noEmptyFrames) 
    dataFileSize = (size_t) (ptr - buffer);

  // Write buffer to file
  outfile->IO->Write(buffer,sizeof(char),dataFileSize);

  // Free buffer
  if (buffer!=NULL) free(buffer);
}

/* DataFile::WriteDataInverted()
 * Alternate method of writing out data where X and Y values are switched. 
 * Each frame is put into a column, with column 1 containing headers.
 */
void DataFile::WriteDataInverted(PtrajFile *outfile) {
  int frame,set,empty;
  int currentLineSize=0;
  int lineSize=0;
  char *buffer,*ptr;

  buffer=NULL;
  for (set=0; set < Nsets; set++) {
    // if specified check for empty frames in the set
    if (noEmptyFrames) {
      empty=0;
      for (frame=0; frame<maxFrames; frame++) {
        if (SetList[set]->isEmpty(frame) ) {empty++; break;}
      }
      if (empty!=0) continue;
    }
    // Necessary buffer size is (set width * number of frames) + newline + NULL
    lineSize = (SetList[set]->Width() * maxFrames) + 2;
    // Check if lineSize > currentLineSize; if so, reallocate buffer
    if (lineSize > currentLineSize) {
      buffer = (char*) realloc(buffer, lineSize * sizeof(char));
      currentLineSize=lineSize;
    }
    // Write header as first column
    outfile->IO->Printf("\"%12s\" ",SetList[set]->Name());
    // Write each frame to a column
    ptr = buffer;
    for (frame=0; frame<maxFrames; frame++) {
      ptr = SetList[set]->Write(ptr,frame);
    }
    // IO->Printf is limited to 1024 chars, use Write instead
    outfile->IO->Write(buffer,sizeof(char),strlen(buffer));
    outfile->IO->Printf("\n");
  }
  // free buffer
  if (buffer!=NULL) free(buffer);
}
 
/* DataFile::WriteGrace() 
 * Write out sets to file in xmgrace format.
 */
void DataFile::WriteGrace(PtrajFile *outfile) {
  int set,frame;
  int lineSize=0;;
  int currentLineSize=0;
  char *buffer;

  // Grace Header
  outfile->IO->Printf("@with g0\n");
  outfile->IO->Printf("@  xaxis label \"%s\"\n",xlabel);
  outfile->IO->Printf("@  yaxis label \"%s\"\n",ylabel);
  outfile->IO->Printf("@  legend 0.2, 0.995\n");
  outfile->IO->Printf("@  legend char size 0.60\n");

  // Loop over sets in data
  buffer=NULL;
  for (set=0; set<Nsets; set++) {
    // Set information
    outfile->IO->Printf("@  s%i legend \"%s\"\n",set,SetList[set]->Name());
    outfile->IO->Printf("@target G0.S%i\n",set);
    outfile->IO->Printf("@type xy\n");
    // Calc buffer size; reallocate if bigger than current size
    lineSize = SetList[set]->Width() + 2;
    if (lineSize > currentLineSize) {
      buffer = (char*) realloc(buffer, lineSize * sizeof(char));
      currentLineSize = lineSize;
      // Since using IO->Printf, buffer cannot be larger than 1024
      if (lineSize>=1024) {
        mprintf("Error: DataFile::WriteGrace: %s: Data width >= 1024\n",filename);
        if (buffer!=NULL) free(buffer);
        return;
      }
    }
    // Set Data
    for (frame=0; frame<maxFrames; frame++) {
      // If specified, run through every set in the frame and check if empty
      if (noEmptyFrames) {
        if ( SetList[set]->isEmpty(frame) ) continue; 
      }
      SetList[set]->Write(buffer,frame);
      outfile->IO->Printf("%8i%s\n",frame+OUTPUTFRAMESHIFT,buffer);
    }
  }
  if (buffer!=NULL) free(buffer);
}

/* DataFile::WriteGraceInverted() 
 * Write out sets to file in xmgrace format. Write out data from each
 * frame as 1 set.
 */
void DataFile::WriteGraceInverted(PtrajFile *outfile) {
  int set,frame,empty;
  int lineSize=0;;
  int currentLineSize=0;
  char *buffer;

  // Grace Header
  outfile->IO->Printf("@with g0\n");
  outfile->IO->Printf("@  xaxis label \"\"\n");
  outfile->IO->Printf("@  yaxis label \"\"\n");
  outfile->IO->Printf("@  legend 0.2, 0.995\n");
  outfile->IO->Printf("@  legend char size 0.60\n");

  // Calculate maximum expected size for output line.
  for (set=0; set < Nsets; set++) {
    lineSize = SetList[set]->Width() + 2;
    if (lineSize > currentLineSize)
      currentLineSize = lineSize;
  }
  // Since using IO->Printf, buffer cannot be larger than 1024
  if (lineSize>=1024) {
    mprintf("Error: DataFile::WriteGraceInverted: %s: Data width >= 1024\n",filename);
    return;
  }
  buffer = (char*) malloc(lineSize * sizeof(char));

  // Loop over frames
  for (frame=0; frame<maxFrames; frame++) {
    // If specified, run through every set in the frame and check if empty
    if (noEmptyFrames) {
      empty=0;
      for (set=0; set<Nsets; set++) {
        if ( SetList[set]->isEmpty(frame) ) {empty++; break;}
      }
      if (empty!=0) continue;
    }

    // Set information
    //outfile->IO->Printf("@  s%i legend \"%s\"\n",set,SetList[set]->Name());
    outfile->IO->Printf("@target G0.S%i\n",frame);
    outfile->IO->Printf("@type xy\n");
    // Loop over all Set Data for this frame
    for (set=0; set<Nsets; set++) {
      SetList[set]->Write(buffer,frame);
      outfile->IO->Printf("%8i%s \"%s\"\n",set+OUTPUTFRAMESHIFT,buffer,SetList[set]->Name());
    }
  }
  if (buffer!=NULL) free(buffer);
}

/* DataFile::WriteGnuplot()
 * Write each frame from all sets in blocks in the following format:
 *   Frame Set   Value
 * Originally there was a -0.5 offset for the Set values in order to center
 * grid lines on the Y values, e.g.
 *   1     0.5   X
 *   1     1.5   X
 *   1     2.5   X
 *
 *   2     0.5   X
 *   2     1.5   X
 *   ...
 * However, in the interest of keeping data consistent, this is no longer
 * done. Could be added back in later as an option.
 */
void DataFile::WriteGnuplot(PtrajFile *outfile) {
  char *buffer;
  int set, frame;
  int lineSize=0;
  int currentLineSize=0;
  //float Fset;
  double xcoord, ycoord;

  // Calculate maximum expected size for output line.
  for (set=0; set < Nsets; set++) {
    lineSize = SetList[set]->Width() + 2;
    if (lineSize > currentLineSize)
      currentLineSize = lineSize;
  }
  // Since using IO->Printf, buffer cannot be larger than 1024
  if (lineSize>=1024) {
    mprintf("Error: DataFile::WriteGnuplot: %s: Data width >= 1024\n",filename);
    return;
  }
  buffer = (char*) malloc(lineSize * sizeof(char));

  if (!useMap)
    outfile->IO->Printf("set pm3d map corners2color c1\n");
  else
    outfile->IO->Printf("set pm3d map\n");

  // Turn off labels if number of sets is too large since they 
  // become unreadable. Should eventually have some sort of 
  // autotick option.
  if (printLabels && Nsets > 30 ) {
    mprintf("Warning: %s: gnuplot - number of sets %i > 30, turning off Y labels.\n",
            filename,Nsets);
    printLabels=false;
  }

  if (printLabels) {
    // NOTE: Add option to turn on grid later?
    //outfile->IO->Printf("set pm3d map hidden3d 100 corners2color c1\n");
    //outfile->IO->Printf("set style line 100 lt 2 lw 0.5\n");
    // Set up Y labels
    outfile->IO->Printf("set ytics %lf,%lf\nset ytics(",ymin,ystep);
    for (set=0; set<Nsets; set++) {
      if (set>0) outfile->IO->Printf(",");
      ycoord = (ystep * set) + ymin;
      outfile->IO->Printf("\"%s\" %lf",SetList[set]->Name(),ycoord);
    }
    outfile->IO->Printf(")\n");
  }

  // Set axis labels
  outfile->IO->Printf("set xlabel \"%s\"\n",xlabel);
  outfile->IO->Printf("set ylabel \"%s\"\n",ylabel);
  // Make Yrange +1 and -1 so entire grid can be seen
  ycoord = (ystep * Nsets) + ymin;
  outfile->IO->Printf("set yrange [%lf:%lf]\n",ymin-ystep,ycoord+ystep);
  // Make Xrange +1 and -1 as well
  xcoord = (xstep * maxFrames) + xmin;
  outfile->IO->Printf("set xrange [%lf:%lf]\n",xmin-xstep,xcoord+xstep);
  // Plot command
  outfile->IO->Printf("splot \"-\" with pm3d title \"%s\"\n",filename);

  // Data
  for (frame=0; frame < maxFrames; frame++) {
    //Fset = OUTPUTFRAMESHIFT;
    //Fset -= 0.5;
    xcoord = (xstep * frame) + xmin;
    for (set=0; set < Nsets; set++) {
      ycoord = (ystep * set) + ymin;
      SetList[set]->Write(buffer,frame);
      outfile->IO->Printf("%lf %lf %s\n",xcoord,ycoord,buffer);
      //Fset++;
    }
    // Print one empty row for gnuplot pm3d
    ycoord = (ystep * set) + ymin;
    outfile->IO->Printf("%lf %lf %8i\n\n",xcoord,ycoord,0);
  }
  // Print one empty set for gnuplot pm3d
  //Fset = OUTPUTFRAMESHIFT;
  //Fset -= 0.5;
  xcoord = (xstep * frame) + xmin;
  for (set=0; set<=Nsets; set++) {
    ycoord = (ystep * set) + ymin;
    outfile->IO->Printf("%lf %lf %8i\n",xcoord,ycoord,0);
    //Fset++;
  }
  outfile->IO->Printf("\n");

  // End and Pause command
  outfile->IO->Printf("end\npause -1\n");
  // Free buffer
  if (buffer!=NULL) free(buffer);
}


