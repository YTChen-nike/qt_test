/*base_64.h�ļ�*/
#ifndef BASE_64_H
#define BASE_64_H
#include <string>
/**
 * Base64 ����/����
 * @author liruixing
 */
class Base64{
private:

    static const char base64_pad = '=';
public:
    static std::string Encode(const unsigned char * str,int bytes);
    static std::string Decode(const char *str,int bytes);
    void Debug(bool open = true);
};
#endif