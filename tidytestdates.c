#include <stdio.h>
#include <tidy.h>
#include <tidybuffio.h>
#include <curl/curl.h>
#include <string.h>

uint write_cb(char *in, uint size, uint nmemb, TidyBuffer *out)
{
  uint r;
  r = size * nmemb;
  tidyBufAppend(out, in, r);
  return r;
}

// function for parsing ACT test date page
void dumpNodeACT(TidyDoc doc, TidyNode tnode, int toPrint)
{
  TidyNode child;
  for(child = tidyGetChild(tnode); child; child = tidyGetNext(child)) {
    ctmbstr name = tidyNodeGetName(child);
    // print the first td in each tr
    if(name) {
      if(strcmp(name, "tr") == 0)
      {
         dumpNodeACT(doc, child, 1);
      }
      if(strcmp(name, "td") == 0 && toPrint == 1)
      {
         dumpNodeACT(doc, child, 2);
         toPrint = 0;
      }
    }
    else if(toPrint == 2){
      TidyBuffer buf;
      tidyBufInit(&buf);
      tidyNodeGetText(doc, child, &buf);
      printf("%s", buf.bp?(char *)buf.bp:"");
      tidyBufFree(&buf);
    }
    dumpNodeACT(doc, child, 0);    
  }
}

// function for parsing SAT test date page
void dumpNodeSAT(TidyDoc doc, TidyNode tnode, int toPrint)
{
  TidyNode child;
  for(child = tidyGetChild(tnode); child; child = tidyGetNext(child)) {
    ctmbstr name = tidyNodeGetName(child);
    // print the bolded td in each tr
    if(name) {
      if(strcmp(name, "td") == 0)
      {
         dumpNodeSAT(doc, child, 1);
      }
      if(strcmp(name, "strong") == 0 && toPrint == 1)
      {
         dumpNodeSAT(doc, child, 2);
      }
    }
    else if(toPrint == 2){
      TidyBuffer buf;
      tidyBufInit(&buf);
      tidyNodeGetText(doc, child, &buf);
      printf("%s", buf.bp?(char *)buf.bp:"");
      tidyBufFree(&buf);
    }
    dumpNodeSAT(doc, child, 0);
  }
}

int main(int argc, char* argv[])
{
  int err;
  char* url = calloc(256, sizeof(char));

  if(argc != 2 || (strcmp(argv[1], "ACT") != 0 && strcmp(argv[1], "SAT") != 0))
  {
     printf("Usage: %s [ACT/SAT]", argv[0]);
     return 1;
  }

  CURL *curl;
  char curl_errbuf[CURL_ERROR_SIZE];
  TidyDoc tdoc;
  TidyBuffer docbuf = {0};
  TidyBuffer tidy_errbuf = {0};
 
  curl = curl_easy_init();

  if(strcmp(argv[1], "SAT") == 0)
  {
    printf("UPCOMING SAT DATES\n");
    url = "https://collegereadiness.collegeboard.org/sat/register/dates-deadlines";
  }
  else
  {
    printf("UPCOMING ACT DATES\n");
    url = "https://www.act.org/content/act/en/products-and-services/the-act/registration.html";
  }
  
  if(curl) {
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, curl_errbuf);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);

    tdoc = tidyCreate();
    tidyOptSetBool(tdoc, TidyForceOutput, yes);
    tidyOptSetInt(tdoc, TidyWrapLen, 4096);
    tidySetErrorBuffer(tdoc, &tidy_errbuf);
    tidyBufInit(&docbuf);

    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &docbuf);

    err = curl_easy_perform(curl);
    if(!err) {
      err = tidyParseBuffer(tdoc, &docbuf);
      if(err >= 0) {
        err = tidyCleanAndRepair(tdoc);
        if(err >= 0) {
          err = tidyRunDiagnostics(tdoc);
          if(err >= 0) {
            if(strcmp(argv[1], "SAT") == 0) dumpNodeSAT(tdoc, tidyGetRoot(tdoc), 0);
            else dumpNodeACT(tdoc, tidyGetRoot(tdoc), 0);

            fprintf(stderr, "%s\n", tidy_errbuf.bp);
          }
        }
      }
    }

    else
      fprintf(stderr, "%s\n", curl_errbuf);

    curl_easy_cleanup(curl);
    tidyBufFree(&docbuf);
    tidyBufFree(&tidy_errbuf);
    tidyRelease(tdoc);
    return err;
  }

}


