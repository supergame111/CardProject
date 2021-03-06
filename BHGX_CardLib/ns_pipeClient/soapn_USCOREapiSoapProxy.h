/* soapn_USCOREapiSoapProxy.h
   Generated by gSOAP 2.7.17 from ns_pipe.h
   Copyright(C) 2000-2010, Robert van Engelen, Genivia Inc. All Rights Reserved.
   This part of the software is released under one of the following licenses:
   GPL, the gSOAP public license, or Genivia's license for commercial use.
*/

#ifndef soapn_USCOREapiSoapProxy_H
#define soapn_USCOREapiSoapProxy_H
#include "soapH.h"
class n_USCOREapiSoap
{   public:
	/// Runtime engine context allocated in constructor
	struct soap *soap;
	/// Endpoint URL of service 'n_USCOREapiSoap' (change as needed)
	const char *endpoint;
	/// Constructor allocates soap engine context, sets default endpoint URL, and sets namespace mapping table
	n_USCOREapiSoap()
	{ soap = soap_new(); endpoint = "http://localhost/nh_webservice/n_api.asmx"; if (soap && !soap->namespaces) { static const struct Namespace namespaces[] = 
{
	{"SOAP-ENV", "http://www.w3.org/2003/05/soap-envelope", "http://www.w3.org/2003/05/soap-envelope", NULL},
	{"SOAP-ENC", "http://www.w3.org/2003/05/soap-encoding", "http://www.w3.org/2003/05/soap-encoding", NULL},
	{"xsi", "http://www.w3.org/2001/XMLSchema-instance", "http://www.w3.org/*/XMLSchema-instance", NULL},
	{"xsd", "http://www.w3.org/2001/XMLSchema", "http://www.w3.org/*/XMLSchema", NULL},
	{"ns2", "http://tempurl.org/n_apiSoap", NULL, NULL},
	{"ns1", "http://tempurl.org", NULL, NULL},
	{"ns3", "http://tempurl.org/n_apiSoap12", NULL, NULL},
	{NULL, NULL, NULL, NULL}
};
	soap->namespaces = namespaces; } };
	/// Destructor frees deserialized data and soap engine context
	virtual ~n_USCOREapiSoap() { if (soap) { soap_destroy(soap); soap_end(soap); soap_free(soap); } };
	/// Invoke 'nh_pipe' of service 'n_USCOREapiSoap' and return error code (or SOAP_OK)
	virtual int __ns2__nh_USCOREpipe(_ns1__nh_USCOREpipe *ns1__nh_USCOREpipe, _ns1__nh_USCOREpipeResponse *ns1__nh_USCOREpipeResponse) { return soap ? soap_call___ns2__nh_USCOREpipe(soap, endpoint, NULL, ns1__nh_USCOREpipe, ns1__nh_USCOREpipeResponse) : SOAP_EOM; };
};
#endif
