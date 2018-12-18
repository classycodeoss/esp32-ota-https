//
//  main.h
//  esp32-ota-https
//
//  Updating the firmware over the air.
//
//  Created by Andreas Schweizer on 11.01.2017.
//  Copyright © 2017 Classy Code GmbH
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of this 
// software and associated documentation files (the "Software"), to deal in the Software 
// without restriction, including without limitation the rights to use, copy, modify, 
// merge, publish, distribute, sublicense, and/or sell copies of the Software, and to 
// permit persons to whom the Software is furnished to do so, subject to the following 
// conditions:
// 
// The above copyright notice and this permission notice shall be included in all copies 
// or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A 
// PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT 
// HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF 
// CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE 
// OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//


#ifndef __MAIN_H__
#define __MAIN_H__ 1


// Adjust these values for your environment.
// -------------------------------------------------------------------------------------

// Used by the OTA module to check if the current version is different from the version
// on the server, i.e. if an upgrade or downgrade should be performed.
#define SOFTWARE_VERSION          1

// Provide the network name and password of your WIFI network.
#define WIFI_NETWORK_SSID         "CC-GUEST"
#define WIFI_NETWORK_PASSWORD     "xxxxxxxxxx"

// Provide server name, path to metadata file and polling interval for OTA updates.
#define OTA_SERVER_HOST_NAME      "www.classycode.io"
#define OTA_SERVER_METADATA_PATH  "/esp32/ota.txt"
#define OTA_POLLING_INTERVAL_S    5
#define OTA_AUTO_REBOOT           1

// Provide the Root CA certificate for chain validation.
// (copied from gd_bundle-g2-g1.crt)
#define OTA_SERVER_ROOT_CA_PEM \
    "-----BEGIN CERTIFICATE-----\n" \
    "MIIEfTCCA2WgAwIBAgIDG+cVMA0GCSqGSIb3DQEBCwUAMGMxCzAJBgNVBAYTAlVT\n" \
    "MSEwHwYDVQQKExhUaGUgR28gRGFkZHkgR3JvdXAsIEluYy4xMTAvBgNVBAsTKEdv\n" \
    "IERhZGR5IENsYXNzIDIgQ2VydGlmaWNhdGlvbiBBdXRob3JpdHkwHhcNMTQwMTAx\n" \
    "MDcwMDAwWhcNMzEwNTMwMDcwMDAwWjCBgzELMAkGA1UEBhMCVVMxEDAOBgNVBAgT\n" \
    "B0FyaXpvbmExEzARBgNVBAcTClNjb3R0c2RhbGUxGjAYBgNVBAoTEUdvRGFkZHku\n" \
    "Y29tLCBJbmMuMTEwLwYDVQQDEyhHbyBEYWRkeSBSb290IENlcnRpZmljYXRlIEF1\n" \
    "dGhvcml0eSAtIEcyMIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAv3Fi\n" \
    "CPH6WTT3G8kYo/eASVjpIoMTpsUgQwE7hPHmhUmfJ+r2hBtOoLTbcJjHMgGxBT4H\n" \
    "Tu70+k8vWTAi56sZVmvigAf88xZ1gDlRe+X5NbZ0TqmNghPktj+pA4P6or6KFWp/\n" \
    "3gvDthkUBcrqw6gElDtGfDIN8wBmIsiNaW02jBEYt9OyHGC0OPoCjM7T3UYH3go+\n" \
    "6118yHz7sCtTpJJiaVElBWEaRIGMLKlDliPfrDqBmg4pxRyp6V0etp6eMAo5zvGI\n" \
    "gPtLXcwy7IViQyU0AlYnAZG0O3AqP26x6JyIAX2f1PnbU21gnb8s51iruF9G/M7E\n" \
    "GwM8CetJMVxpRrPgRwIDAQABo4IBFzCCARMwDwYDVR0TAQH/BAUwAwEB/zAOBgNV\n" \
    "HQ8BAf8EBAMCAQYwHQYDVR0OBBYEFDqahQcQZyi27/a9BUFuIMGU2g/eMB8GA1Ud\n" \
    "IwQYMBaAFNLEsNKR1EwRcbNhyz2h/t2oatTjMDQGCCsGAQUFBwEBBCgwJjAkBggr\n" \
    "BgEFBQcwAYYYaHR0cDovL29jc3AuZ29kYWRkeS5jb20vMDIGA1UdHwQrMCkwJ6Al\n" \
    "oCOGIWh0dHA6Ly9jcmwuZ29kYWRkeS5jb20vZ2Ryb290LmNybDBGBgNVHSAEPzA9\n" \
    "MDsGBFUdIAAwMzAxBggrBgEFBQcCARYlaHR0cHM6Ly9jZXJ0cy5nb2RhZGR5LmNv\n" \
    "bS9yZXBvc2l0b3J5LzANBgkqhkiG9w0BAQsFAAOCAQEAWQtTvZKGEacke+1bMc8d\n" \
    "H2xwxbhuvk679r6XUOEwf7ooXGKUwuN+M/f7QnaF25UcjCJYdQkMiGVnOQoWCcWg\n" \
    "OJekxSOTP7QYpgEGRJHjp2kntFolfzq3Ms3dhP8qOCkzpN1nsoX+oYggHFCJyNwq\n" \
    "9kIDN0zmiN/VryTyscPfzLXs4Jlet0lUIDyUGAzHHFIYSaRt4bNYC8nY7NmuHDKO\n" \
    "KHAN4v6mF56ED71XcLNa6R+ghlO773z/aQvgSMO3kwvIClTErF0UZzdsyqUvMQg3\n" \
    "qm5vjLyb4lddJIGvl5echK1srDdMZvNhkREg5L4wn3qkKQmw4TRfZHcYQFHfjDCm\n" \
    "rw==\n" \
    "-----END CERTIFICATE-----\n"

// Provide the Peer certificate for certificate pinning.
// (copied from classycode.io.crt)
#define OTA_PEER_PEM \
    "-----BEGIN CERTIFICATE-----\n" \
    "MIIFNzCCBB+gAwIBAgIJANYDzCSwNgryMA0GCSqGSIb3DQEBCwUAMIG0MQswCQYD\n" \
    "VQQGEwJVUzEQMA4GA1UECBMHQXJpem9uYTETMBEGA1UEBxMKU2NvdHRzZGFsZTEa\n" \
    "MBgGA1UEChMRR29EYWRkeS5jb20sIEluYy4xLTArBgNVBAsTJGh0dHA6Ly9jZXJ0\n" \
    "cy5nb2RhZGR5LmNvbS9yZXBvc2l0b3J5LzEzMDEGA1UEAxMqR28gRGFkZHkgU2Vj\n" \
    "dXJlIENlcnRpZmljYXRlIEF1dGhvcml0eSAtIEcyMB4XDTE2MDcxMTE3NDE0MloX\n" \
    "DTE5MDcxMTE3NDE0MlowPzEhMB8GA1UECxMYRG9tYWluIENvbnRyb2wgVmFsaWRh\n" \
    "dGVkMRowGAYDVQQDExF3d3cuY2xhc3N5Y29kZS5pbzCCASIwDQYJKoZIhvcNAQEB\n" \
    "BQADggEPADCCAQoCggEBAPKdu6d89pXjP0qVWG33xGSOyNRLak9zShtx/KaO9ftj\n" \
    "TmBASCBlxk0aNba6gt1H+Hx2NPZqCd8BWRM/nt1fk9cCYYsRkjrJn/o8i0LmQTyr\n" \
    "2jb/LVScQEApaCe3sNA7evz+F2jNodX45uvi+wiPoUZkoG3aToPkx3QjrphMGD1g\n" \
    "lS2zSVy6Zg0AcnroHCMSdSlxo2DOHie8zfAyCQacyBjDNrZgcVZzIANyw6Y/YAeR\n" \
    "bQwzDerm23daSMdkfc6ewsLvFmfsy7VZIdJYsPOOA7o0xyeeUW1C4NnWxvp8j4tR\n" \
    "1X/JWSNAYki8pj2ChwUF6SPxwHMDnp//g1liuCR7Mh0CAwEAAaOCAb4wggG6MAwG\n" \
    "A1UdEwEB/wQCMAAwHQYDVR0lBBYwFAYIKwYBBQUHAwEGCCsGAQUFBwMCMA4GA1Ud\n" \
    "DwEB/wQEAwIFoDA3BgNVHR8EMDAuMCygKqAohiZodHRwOi8vY3JsLmdvZGFkZHku\n" \
    "Y29tL2dkaWcyczEtMjY1LmNybDBdBgNVHSAEVjBUMEgGC2CGSAGG/W0BBxcBMDkw\n" \
    "NwYIKwYBBQUHAgEWK2h0dHA6Ly9jZXJ0aWZpY2F0ZXMuZ29kYWRkeS5jb20vcmVw\n" \
    "b3NpdG9yeS8wCAYGZ4EMAQIBMHYGCCsGAQUFBwEBBGowaDAkBggrBgEFBQcwAYYY\n" \
    "aHR0cDovL29jc3AuZ29kYWRkeS5jb20vMEAGCCsGAQUFBzAChjRodHRwOi8vY2Vy\n" \
    "dGlmaWNhdGVzLmdvZGFkZHkuY29tL3JlcG9zaXRvcnkvZ2RpZzIuY3J0MB8GA1Ud\n" \
    "IwQYMBaAFEDCvSeOzDSDMKIz1/tss/C0LIDOMCsGA1UdEQQkMCKCEXd3dy5jbGFz\n" \
    "c3ljb2RlLmlvgg1jbGFzc3ljb2RlLmlvMB0GA1UdDgQWBBRSUKeJNe7VqzcoU22Q\n" \
    "+2Cwm5LqjjANBgkqhkiG9w0BAQsFAAOCAQEAO4P8Pxbfdspwtg0zBi+d6AHkBEA8\n" \
    "lDa/S6XGx+1fCbtDAm734eVvHDRQOIUtJjN8RI0vsV75Qn5M0hfm9l8BdTp/KZgI\n" \
    "3yDAWmHp6djQs35vVHs00aFoyB2BHYgJ0/mdlnmwFeqYo0lJn3MrAT+mZ824KDKN\n" \
    "SjOhhHUy2HFaWQ8p6t+r8Gwexc2UJVXQ5+0dIEVHb3E3ZYWkebG7yMZiJvpt6fgZ\n" \
    "L2DbU95sIPiNCxICtTdf6BcU31JrIiBqozKenXs4cIEj4YBE6QVsiS8zJt/K3Sc5\n" \
    "anfkm8wWGaEfaD16wcz7+7yi937RE1Q2EdyKMVhx4UNTln2HqxxIqmUSYQ==\n" \
    "-----END CERTIFICATE-----\n"

// -------------------------------------------------------------------------------------


#endif // __MAIN_H__
