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

void dumpNode(TidyDoc doc, TidyNode tnode, int toPrint)
{
  TidyNode child;
  for(child = tidyGetChild(tnode); child; child = tidyGetNext(child)) {
    ctmbstr name = tidyNodeGetName(child);
    if(name) {
      if(strcmp(name, "td") == 0)
      {
         dumpNode(doc, child, 1);
      }
      if(strcmp(name, "strong") == 0 && toPrint == 1)
      {
         dumpNode(doc, child, 2);
      }
    }
    else if(toPrint == 2){
      TidyBuffer buf;
      tidyBufInit(&buf);
      tidyNodeGetText(doc, child, &buf);
      printf("%s", buf.bp?(char *)buf.bp:"");
      tidyBufFree(&buf);
    }
    dumpNode(doc, child, 0);
  }
}

int main()
{
  int err;
  char* satUrl = "https://collegereadiness.collegeboard.org/sat/register/dates-deadlines";

  CURL *curl;
  char curl_errbuf[CURL_ERROR_SIZE];
  TidyDoc tdoc;
  TidyBuffer docbuf = {0};
  TidyBuffer tidy_errbuf = {0};
 
  curl = curl_easy_init();

  printf("Upcoming SAT dates: \n");
  if(curl) {
    curl_easy_setopt(curl, CURLOPT_URL, satUrl);
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
            dumpNode(tdoc, tidyGetRoot(tdoc), 0);
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


