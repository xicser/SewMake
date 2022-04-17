/*
http报文解析器
*/

#include "http_msg_parser.h"
#include <vector>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
 
/* 协议解析 */
void HttpMsgParser::tryDecode(const string& buf) 
{
    this->parseInternal(buf.c_str(),buf.size());
}

void HttpMsgParser::tryDecode(const char *buf, int size)
{
	this->parseInternal(buf, size);
}
 
/* 请求方法 */
const string& HttpMsgParser::getMethod() const
{
    return _method;
}
 
/* 请求URL */
const string& HttpMsgParser::getUrl() const
{
    return _url;
}
 
/* 请求头key->value */
const unordered_map<string, string>& HttpMsgParser::getRequestParams() const
{
    return _requestParams;
}
 
/* 请求URL参数 */
const string& HttpMsgParser::getProtocol() const
{
    return _protocol;
}
 
/* 请求协议版本 */
const string& HttpMsgParser::getVersion() const
{
    return _version;
}
 
/* 请求头 */
const unordered_map<string, string>& HttpMsgParser::getHeaders() const
{
    return _header;
}
 
/* 请求主体 */
const string& HttpMsgParser::getBody() const
{
    return _body;
}
 
/* 解析请求行 */
void HttpMsgParser::parseInternal(const char* buf,int size) 
{
    StringBuffer method;
    StringBuffer url;
 
    StringBuffer requestParamsKey;
    StringBuffer requestParamsValue;
 
    StringBuffer protocol;
    StringBuffer version;
 
    StringBuffer headerKey;
    StringBuffer headerValue;
 
    int bodyLength=0;
    char* p0=const_cast<char*>(buf+_nextPos);//去掉const限制
    while(_decodeState!=HttpRequestDecodeState::INVALID
           &&_decodeState!=HttpRequestDecodeState::INVALID_METHOD
           &&_decodeState!=HttpRequestDecodeState::INVALID_URI
           &&_decodeState!=HttpRequestDecodeState::INVALID_VERSION
           &&_decodeState!=HttpRequestDecodeState::INVALID_HEADER
           &&_decodeState!=HttpRequestDecodeState::COMPLEIE
           &&_nextPos<size
         )
    {
        char ch=*p0;    //当前的字符
        char* p=p0++;   //指针偏移 
        int scanPos=_nextPos++;//下一个指针往后偏移
 
        switch(_decodeState)
        {
            case HttpRequestDecodeState::START:
            {
                //空格、换行、回车都继续
                if(ch==CR||ch==LF||isblank(ch)){}
                else if(isupper(ch)) //判断是不是大写字符
                {
                    method.begin=p;//请求方法的起始点
                    _decodeState=HttpRequestDecodeState::METHOD;//遇到第一个字符开始解析方法
                }
                else
                {
                    _decodeState=HttpRequestDecodeState::INVALID;//无效的字符
                }
                break;
            }
            case HttpRequestDecodeState::METHOD:
            {
                //请求方法需要大写,大写字母就继续
                if(isupper(ch)){}
                else if(isblank(ch))//空格说明请求方法解析结束,下一步解析URI
                {
                    method.end=p;//请求方法解析结束
                    _method=method;
                    _decodeState=HttpRequestDecodeState::BEFORE_URI;
                }
                else
                {
                    _decodeState=HttpRequestDecodeState::INVALID_METHOD;//无效的请求方法
                }
                break;
            }
            case HttpRequestDecodeState::BEFORE_URI:
            {
                /*请求URL连接前的处理,需要'/'开头*/
                if(isblank(ch)){}
                else if(ch=='/')
                {
                    url.begin=p;
                    _decodeState=HttpRequestDecodeState::IN_URI;
                }
                else
                {
                    _decodeState=HttpRequestDecodeState::INVALID_URI;
                }
                break;
            }
            case HttpRequestDecodeState::IN_URI:
            {
                /*开始处理请求路径URL的字符串*/
                if(isblank(ch))
                {
                    url.end=p;
                    _url=url;
                    _decodeState=HttpRequestDecodeState::BEFORE_PROTOCAL;
                }
                else if(ch=='?')
                {
                    url.end=p;
                    _url=url;
                    _decodeState=HttpRequestDecodeState::BEFORE_URI_PARAM_KEY;
                }
                else{}
                break;
            }
            case HttpRequestDecodeState::BEFORE_URI_PARAM_KEY:
            {
                if(isblank(ch)||ch==LF||ch==CR)//'?'后面是空格、回车、换行则是无效的URL
                {
                    _decodeState=HttpRequestDecodeState::INVALID_URI;
                }
                else
                {
                    requestParamsKey.begin=p;
                    _decodeState=HttpRequestDecodeState::URI_PARAM_KEY;//URL的参数key
                }
                break;
            }
            case HttpRequestDecodeState::URI_PARAM_KEY:
            {
                if(ch=='=')
                {
                    requestParamsKey.end=p;
                    _decodeState=HttpRequestDecodeState::BEFORE_URI_PARAM_VALUE;//开始解析参数值
                }
                else if(isblank(ch))
                {
                    _decodeState=HttpRequestDecodeState::INVALID_URI;//无效的请求参数
                }
                else{}
                break;
            }
            case HttpRequestDecodeState::BEFORE_URI_PARAM_VALUE:
            {
                if(isblank(ch)||ch==LF||ch==CR)
                {
                    _decodeState=HttpRequestDecodeState::INVALID_URI;
                }
                else
                {
                    requestParamsValue.begin=p;
                    _decodeState=HttpRequestDecodeState::URI_PARAM_VALUE;
                }
                break;
            }
            case HttpRequestDecodeState::URI_PARAM_VALUE:
            {
                if(ch=='&')
                {
                    requestParamsValue.end=p;
                    //收获一个请求参数
                    _requestParams.insert({requestParamsKey,requestParamsValue});
                    _decodeState=HttpRequestDecodeState::BEFORE_URI_PARAM_KEY;
                }
                else if(isblank(ch))
                {
                    requestParamsValue.end=p;
                    //空格也收获一个请求参数
                    _requestParams.insert({requestParamsKey,requestParamsValue});
                    _decodeState=HttpRequestDecodeState::BEFORE_PROTOCAL;
                }
                else{}
                break;
            }
            case HttpRequestDecodeState::BEFORE_PROTOCAL:
            {
                if(isblank(ch)){}
                else
                {
                    protocol.begin=p;
                    _decodeState=HttpRequestDecodeState::PROTOCAL;
                }
                break;
            }
            case HttpRequestDecodeState::PROTOCAL:
            {
                //解析请求协议
                if(ch=='/')
                {
                    protocol.end=p;
                    _protocol=protocol;
                    _decodeState=HttpRequestDecodeState::BEFORE_VERSION;
                }
                else{}
                break;
            }
            case HttpRequestDecodeState::BEFORE_VERSION:
            {
                if(isdigit(ch))
                {
                    version.begin=p;
                    _decodeState=HttpRequestDecodeState::VERSION;
                }
                else
                {
                    _decodeState=HttpRequestDecodeState::INVALID_VERSION;
                }
                break;
            }
            case HttpRequestDecodeState::VERSION:
            {
                //协议版本解析,如果不是数字,则协议版本不对
                if(ch==CR)
                {
                    version.end=p;
                    _version=version;
                    _decodeState=HttpRequestDecodeState::WHEN_CR;//遇到一个回车
                }
                else if(ch=='.')
                {
                    //遇到版本分割
                    _decodeState=HttpRequestDecodeState::VERSION_SPLIT;
                }
                else if(isdigit(ch)){}
                else
                {
                    _decodeState=HttpRequestDecodeState::INVALID_VERSION;
                }
                break;
            }
            case HttpRequestDecodeState::VERSION_SPLIT:
            {
                //遇到版本分隔符之后的字符必须是数字,其它情况都是错误的
                if(isdigit(ch))
                {
                    _decodeState=HttpRequestDecodeState::VERSION;
                }
                else
                {
                    _decodeState=HttpRequestDecodeState::INVALID_VERSION;
                }
            }
            case HttpRequestDecodeState::HEADER_KEY:
            {
                //冒号前后可能有空格
                if(isblank(ch))
                {
                    headerKey.end=p;
                    _decodeState=HttpRequestDecodeState::HEADER_BEFORE_COLON;//冒号之前的状态
                }
                else if(ch==':')
                {
                    headerKey.end=p;
                    _decodeState=HttpRequestDecodeState::HEADER_AFTER_COLON;//冒号之后的状态
                }
                else{}
                break;
            }
            case HttpRequestDecodeState::HEADER_BEFORE_COLON:
            {
                if(isblank(ch)){}
                else if(ch==':')
                {
                    _decodeState=HttpRequestDecodeState::HEADER_AFTER_COLON;
                }
                else
                {
                    //冒号之前的状态不能是空格之外的其他字符
                    _decodeState=HttpRequestDecodeState::INVALID_HEADER;
                }
                break;
            }
            case HttpRequestDecodeState::HEADER_AFTER_COLON:
            {
                if(isblank(ch)){}
                else
                {
                    headerValue.begin=p;
                    _decodeState=HttpRequestDecodeState::HEADER_VALUE;//开始处理值
                }
                break;
            }
            case HttpRequestDecodeState::HEADER_VALUE:
            {
                if(ch==CR)
                {
                    headerValue.end=p;
                    _header.insert({headerKey,headerValue});
                    _decodeState=HttpRequestDecodeState::WHEN_CR;//回车
                }
                break;
            }
            case HttpRequestDecodeState::WHEN_CR:
            {
                if(ch==LF)
                {
                    //前面是回车,如果当前是换行,可换成下一个
                    _decodeState=HttpRequestDecodeState::CR_LF;
                }
                else
                {
                    _decodeState=HttpRequestDecodeState::INVALID;
                }
                break;
            }
            case HttpRequestDecodeState::CR_LF:
            {
                if(ch==CR)
                {
                    //如果在CR_LF状态之后还是CR,可能是到请求体了
                    _decodeState=HttpRequestDecodeState::CR_LF_CR;
                }
                else if(isblank(ch))
                {
                    _decodeState=HttpRequestDecodeState::INVALID;
                }
                else
                {
                    //如果不是,那么就可能又是请求头了
                    headerKey.begin=p;
                    _decodeState=HttpRequestDecodeState::HEADER_KEY;
                }
                break;
            }
            case HttpRequestDecodeState::CR_LF_CR:
            {
                if(ch==LF)
                {
                    //如果是\r接着\n,那么判断是不是需要解析请求体
                    if(_header.count("Content-Length"))
                    {
                        bodyLength=atoi(_header["Content-Length"].c_str());
                        if(bodyLength>0)
                        {
                            _decodeState=HttpRequestDecodeState::BODY;//解析请求体
                        }
                        else
                        {
                            _decodeState=HttpRequestDecodeState::COMPLEIE;//完成了
                        }
                    }
                    else
                    {
                        if(scanPos<size)
                        {
                            bodyLength=size-scanPos;
                            _decodeState=HttpRequestDecodeState::BODY;//解析请求体
                        }
                        else
                        {
                            _decodeState=HttpRequestDecodeState::COMPLEIE;
                        }
                    }
                }
                else
                {
                    _decodeState=HttpRequestDecodeState::INVALID_HEADER;
                }
                break;
            }
            case HttpRequestDecodeState::BODY:
            {
                //解析请求体
                _body.assign(p,bodyLength);
                _decodeState=HttpRequestDecodeState::COMPLEIE;
                break;
            }
            default:break;
        }
    }
}

