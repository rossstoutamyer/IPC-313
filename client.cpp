/*
    Tanzir Ahmed
    Department of Computer Science & Engineering
    Texas A&M University
    Date  : 2/8/20
	Original author of the starter code
	
	Please include your name and UIN below
	Name: Ross Stoutamyer
	UIN: 128006101
 */
#include "common.h"
#include "FIFOreqchannel.h"

using namespace std;


int main(int argc, char *argv[]){
 
	
	int opt;
	int p = 1;
	double t = 0.0;
	int e = 1;
	bool cVal = false;
	bool moreData = false;
	int mVal = 256;
	string stringM = "256";
	string filename = "";
	
	while ((opt = getopt(argc, argv, "p:t:e:f:m:c")) != -1) {
		switch (opt) {
			case 'p':
				p = atoi (optarg);
				moreData = true;
				break;
			case 't':
				t = atof (optarg);
				moreData = true;
				break;
			case 'e':
				e = atoi (optarg);
				moreData = true;
				break;
			case 'f':
				filename = optarg;
				break;
		        case 'c':
			        cVal = true;
				break;
		        case 'm':
			        mVal = atoi (optarg);
				stringM = optarg;
				break;
		}
	}

	pid_t pid = fork();
	if (pid == -1) {
	  cout << "ERROR IN FORK" << endl;
	} else if (pid == 0) {
	    char *childArgs[] = {"./server", "-m", (char*)stringM.c_str(), NULL};
	    execvp(childArgs[0], childArgs);
	}
	
        FIFORequestChannel chan ("control", FIFORequestChannel::CLIENT_SIDE);
	clock_t start = clock();
    // sending a non-sense message, you need to change this
	int buffcap = mVal;
    char buf [buffcap]; // 256
    datamsg x (p, t, e);
    chan.cwrite (&x, sizeof(datamsg)); // question
    double reply;
    double reply2;
    int nbytes = chan.cread (&reply, sizeof(double)); //answer

    if (moreData) {
      cout << "Receiving data from server..." << endl;
    
      cout << "For person " << x.person << ", at time " << x.seconds << ", the value of ecg "<< e <<" is " << reply << endl;
      if (x.ecgno != 1) {
	x.ecgno = 1;
      }
      ofstream newFile;
      newFile.open("received/x1.csv");
      for (double i = 0; i < 60; i += 0.06) {//go through the entire files
        x.seconds = i;

	for (int j = 0; j < 2; ++j) {
	  if (j ==1) {
	    reply = reply2;
	    x.ecgno = 2;
	  }
	  chan.cwrite (&x, sizeof (datamsg)); // question
	  nbytes = chan.cread (&reply2, sizeof(double)); //answer
	  x.ecgno = 1;
	}

	newFile << x.seconds << "," << reply << "," << reply2 << "\n";
	
      }
      newFile.close();
    }

    
	
    filemsg fm (0,0); //create a filemessage with offset 0 and length 0

	string fname = filename;
	int len = sizeof (filemsg) + fname.size()+1;
	
	char buf2 [len];
	memcpy (buf2, &fm, sizeof (filemsg)); //copies sizeof(filemsg) characters from fm into buf2
	strcpy (buf2 + sizeof (filemsg), fname.c_str()); //copies string from fname.c_str() to buf2 + sizeof(filemsg)
	chan.cwrite (buf2, len);  // I want the file length;

	__int64_t freply;
	chan.cread(&freply, sizeof(__int64_t)); //finds size of file and stores into freply
	if (filename != "") {
	  
	  string createdFile = "received/" + filename;
	
	  FILE* fp = fopen (createdFile.c_str(), "wb"); //creates file in received folder to stored data
	  int i = 0;
	  while (freply > 0) { //while the file is not done
	    if (i > 0) { //find offset
	      fm.offset += buffcap;
	    }else {
	      fm.offset = 0;
	    }
	    if (freply >= buffcap) { //if the file length is greater than max message, the length we want is max message
	      fm.length = buffcap;
	    } else {
	      fm.length = freply;
	    }
	    memcpy (buf2, &fm, sizeof (filemsg)); //copies sizeof(filemsg) characters from fm into buf2
	    strcpy (buf2 + sizeof (filemsg), fname.c_str()); //copies string from fname.c_str() to buf2 + sizeof(filemsg)
	    chan.cwrite(&buf2, len);
	    char buf3[buffcap];
	    int fileBytes = chan.cread(&buf3, buffcap);
	    fwrite(buf3, 1, fileBytes, fp);
	    freply -= buffcap;
	    ++i;
	  }

	  fclose(fp);
	}

        

	if (cVal) {
	  char newBuffer[buffcap];
	  string cReply = "";
	  MESSAGE_TYPE newMsg = NEWCHANNEL_MSG;

	  chan.cwrite(&newMsg, sizeof(MESSAGE_TYPE));
	  chan.cread(&newBuffer, buffcap);
	  cReply = string(newBuffer);
	  
	  FIFORequestChannel newChan (cReply, FIFORequestChannel::CLIENT_SIDE);

	  cout << "New channel created! Name: " << cReply << endl;
	  cout << "Using new channel to collect ecgno for 5 people..." << endl;

	  datamsg d (p, t, e);
	  double ncReply;
	  double ncReply2;
	  for (int k = 0; k < 5; ++k) {
	    d.person = k + 1;
	    for (int cnt = 0; cnt < 2; ++cnt) {
	      if (cnt == 1) {
		ncReply = ncReply2;
		d.ecgno = 2;
	      }
	      newChan.cwrite (&d, sizeof (datamsg));
	      int totalBytes = newChan.cread (&ncReply2, sizeof(double));
	      d.ecgno = 1;
	    }
	    
	    cout << "For person " << d.person <<", at time " << d.seconds << ", the value of ecg 1 is " << ncReply << " and ecg 2 is " << ncReply2 << endl;
	  }

	  MESSAGE_TYPE m = QUIT_MSG;
	  newChan.cwrite (&m, sizeof (MESSAGE_TYPE));
	}

	 cout << "Total time in clock ticks " << clock() - start << endl;

	
	
	// closing the channel    
    MESSAGE_TYPE m = QUIT_MSG;
    chan.cwrite (&m, sizeof (MESSAGE_TYPE));
    sleep(2);
}