#include "rpc_server.h"
#include "rpc_method.h"
#include "rpc_protocol.h"
#include "rpc_result.h"
#include "rpc_session.h"
#include <lyon/address.h>
#include <lyon/log.h>
#include <lyon/serialize/serializer.h>
#include <lyon/socket.h>
namespace lyon::rpc {

static Logger::ptr g_logger = LYON_LOG_GET_LOGGER("system");

RpcServer::RpcServer(IOManager *worker, IOManager *ioworker,
                     IOManager *acceptWorker)
    : TcpServer(worker, ioworker, acceptWorker) {}

bool RpcServer::start() {
    if (!m_registerSession || !m_registerSession->isConnected()) {
        for (auto &method : m_methods) {
            registMethodToRegister(method.second);
        }
    }
    return TcpServer::start();
}

bool RpcServer::bindRegister(Address::ptr addr) {
    Socket::ptr sock = Socket::CreateTCP(addr);
    if (!sock->connect(addr)) {
        LYON_LOG_ERROR(g_logger) << "RpcServer::bindRegister fail";
        m_registerSession = nullptr;
        return false;
    }
    m_registerSession = std::make_shared<RpcSession>(sock);
    return true;
}

bool RpcServer::bindRegister(const std::string &host) {
    Address::ptr addr = Address::LookUpAnyIpAddress(host);
    if (!addr) {
        LYON_LOG_ERROR(g_logger) << "RpcServer::bindRegister fail";
        return false;
    }
    return bindRegister(addr);
}

void RpcServer::handleClient(Socket::ptr sock) {
    //新建一个session
    RpcSession::ptr session = std::make_shared<RpcSession>(sock);

    //重复收发消息
    do {
        //接收请求
        RpcProtocol::ptr request = session->recvRpcProtocol();
        //如果请求接收或解析失败
        if (!request) {
            LYON_LOG_ERROR(g_logger) << "RpcServer::handleClient Bad Request";
            break;
        }

        LYON_LOG_DEBUG(g_logger)
            << "Server recv Request = " << request->toString();
        RpcProtocol::ptr response;
        if (request->getType() ==
            RpcProtocol::MSG_TYPE::RPC_CALL_METHOD_REQUEST) {
            //如果是函数调用请求
            response = handleMethodRequest(request);
        }
        LYON_LOG_DEBUG(g_logger)
            << "Server send Response = " << response->toString();

        if (response) {
            session->sendRpcProtocol(response);
        }
        if (!session->isConnected()) {
            break;
        }
    } while (true);
    session->close();
}

RpcProtocol::ptr RpcServer::handleMethodRequest(RpcProtocol::ptr request) {
    //先将消息体转化为序列化参数
    Serializer args(request->getContent(), request->isCompress());
    std::string method_name;
    std::string rt_type;
    std::string args_type;
    //读取函数名
    args >> method_name >> rt_type >> args_type;
    //创建对应的响应
    RpcProtocol::ptr response =
        RpcProtocol::CreateCallMethodResponse(request->getSeqId());

    //响应结果的封装
    std::unique_ptr<RpcResult<>> result;

    if (!m_methods.contains(method_name)) {
        result = std::make_unique<RpcResult<>>(RpcResultState::METHOD_NOT_FOUND,
                                               "method not found");
    } else {
        RpcMethod::ptr method = m_methods[method_name];

        //参数或返回值类型不匹配也视为未找到对应方法
        if (args_type != method->getArgsType() ||
            rt_type != method->getRtType()) {
            result = std::make_unique<RpcResult<>>(
                RpcResultState::METHOD_NOT_FOUND, "method not found");
        }
        //调用函数
        else if (method->call(args)) {
            result = std::make_unique<RpcResult<>>(RpcResultState::OK, "OK");
            //将处理结果的封装和函数的返回值放入响应体中
            response->setContent(result->toString() + args.toString());
            return response;
        } else {
            result = std::make_unique<RpcResult<>>(
                RpcResultState::METHOD_RUN_ERROR, "method run error");
        }
    }
    //其余调用失败的的时候，就不需要仿佛处理结果了。
    response->setContent(result->toString());
    return response;
}

void RpcServer::registMethod(RpcMethod::ptr method) {
    m_methods.emplace(method->getName(), method);
}

void RpcServer::registMethodToRegister(RpcMethod::ptr method) {
    if (!m_registerSession || !m_registerSession->isConnected()) {
        return;
    }
    RpcProtocol::ptr request = RpcProtocol::CreateRegistMethodRequest();

    Serializer ser;

    ser << method->getName() << method->getRtType() << method->getArgsType();

    request->setContent(ser.toString());

    m_registerSession->sendRpcProtocol(request);

    RpcProtocol::ptr response = m_registerSession->recvRpcProtocol();

    // TODO:接收响应并处理。
}

RpcMethod::ptr RpcServer::getMethod(const std::string &name) {
    auto itr = m_methods.find(name);
    if (itr == m_methods.end()) {
        return nullptr;
    }
    return itr->second;
}

} // namespace lyon::rpc
