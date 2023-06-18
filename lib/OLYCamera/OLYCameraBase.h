#ifndef OLYCAMERABASE_H
#define OLYCAMERABASE_H

#include <HTTPClient.h>
#include <tinyxml2.h>
#include <WString.h>

const unsigned int OLYCAMERAERROR_SUCCESS = 0;
const unsigned int OLYCAMERAERROR_HTTP = 1;
const unsigned int OLYCAMERAERROR_XML = 1;

class OLYCameraBase
{
  public:
    const String baseUrl = "http://192.168.0.10/";
    const unsigned int getLastError() { return lastError; }
    const String getLastErrorMessage() { return lastErrorMessage; }

  protected:
    unsigned int lastError = OLYCAMERAERROR_SUCCESS;
    String lastErrorMessage = "";
    String httpGet(String url, const int normalResponseCode = 200);
    String getRootXmlText(String xml);
    HTTPClient httpClient;
};

#endif // OLYCAMERABASE_H