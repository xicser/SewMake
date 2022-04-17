/*
http报文解析器
*/

#ifndef __HTTP_MSG_PARSER_H__
#define __HTTP_MSG_PARSER_H__
 
#include <iostream>
#include <string>
#include <unordered_map>

using namespace std;

#define CR '\r' //回车
#define LF '\n' //换行
 
/*字符串buf,只存储对应的指针,不存储实际的内容*/
struct StringBuffer
{
    char* begin=NULL;//字符串起始位置
    char* end=NULL;  //字符串结束位置
    operator std::string()const{//转移operator,返回类实例化得到string类型的
        return string(begin,end);
    }
};
 
/*HTTP请求行的状态*/
enum HttpRequestDecodeState
{
    INVALID,                //无效
    INVALID_METHOD,         //无效请求方法
    INVALID_URI,            //无效的请求路径
    INVALID_VERSION,        //无效的协议版本号
    INVALID_HEADER,         //无效请求头
 
    START,                  //请求行开始
    METHOD,                 //请求方法
 
    BEFORE_URI,             //请求连接前的状态,需要'/'开头
    IN_URI,                 //url处理
    BEFORE_URI_PARAM_KEY,   //URL请求参数键之前
    URI_PARAM_KEY,          //URL请求参数键
    BEFORE_URI_PARAM_VALUE, //URL的参数值之前
    URI_PARAM_VALUE,        //URL请求参数值
 
    BEFORE_PROTOCAL,        //协议解析之前
    PROTOCAL,               //协议
 
    BEFORE_VERSION,         //版本开始之前
    VERSION_SPLIT,          //版本分割符号'.'
    VERSION,                //版本
 
    HEADER_KEY,
    HEADER_BEFORE_COLON,    //请求头冒号之前
    HEADER_AFTER_COLON,     //请求头冒号之后
    HEADER_VALUE,           //值
 
    WHEN_CR,                //遇到一个回车
    CR_LF,                  //回车换行
    CR_LF_CR,               //回车换行之后
 
    BODY,                   //请求体
    COMPLEIE                //完成
};
 
/*HTTP请求类*/
class HttpMsgParser
{
public:
    void tryDecode(const string& buf);
	void tryDecode(const char *buf, int size);
    const string& getMethod() const;
    const string& getUrl() const;
    const unordered_map<string, string>& getRequestParams() const;
    const string& getProtocol() const;
    const string& getVersion() const;
    const unordered_map<string, string>& getHeaders() const;
    const string& getBody() const;
 
private:
    void parseInternal(const char* buf,int size);
 
private:
    string _method;                                       //请求方法
    string _url;                                          //请求路径（不包含参数）
    unordered_map<string, string> _requestParams;		  //请求参数
    string _protocol;                                     //协议
    string _version;                                      //版本
    unordered_map<string, string> _header;       		  //所有的请求头
    string _body;                                         //请求体
    int _nextPos=0;                                            //下一个位置的
    HttpRequestDecodeState _decodeState=HttpRequestDecodeState::START;//解析状态
};
#endif

