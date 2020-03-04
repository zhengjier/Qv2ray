﻿#include "CoreUtils.hpp"

#include "common/QvHelpers.hpp"
#include "core/handler/ConfigHandler.hpp"

namespace Qv2ray::core
{
    bool IsComplexConfig(const ConnectionId &id)
    {
        return IsComplexConfig(ConnectionManager->GetConnectionRoot(id));
    }
    bool IsComplexConfig(const CONFIGROOT &root)
    {
        bool cRouting = root.contains("routing");
        bool cRule = cRouting && root["routing"].toObject().contains("rules");
        bool cRules = cRule && root["routing"].toObject()["rules"].toArray().count() > 0;
        //
        bool cInbounds = root.contains("inbounds");
        bool cInboundCount = cInbounds && root["inbounds"].toArray().count() > 0;
        //
        bool cOutbounds = root.contains("outbounds");
        bool cOutboundCount = cOutbounds && root["outbounds"].toArray().count() > 1;
        return cRules || cInboundCount || cOutboundCount;
    }

    bool GetOutboundInfo(const OUTBOUND &out, QString *host, int *port, QString *protocol)
    {
        // Set initial values.
        *host = QObject::tr("N/A");
        *port = 0;
        *protocol = out["protocol"].toString(QObject::tr("N/A")).toLower();

        if (*protocol == "vmess")
        {
            auto Server =
                StructFromJsonString<VMessServerObject>(JsonToString(out["settings"].toObject()["vnext"].toArray().first().toObject()));
            *host = Server.address;
            *port = Server.port;
            return true;
        }
        else if (*protocol == "shadowsocks")
        {
            auto x = JsonToString(out["settings"].toObject()["servers"].toArray().first().toObject());
            auto Server = StructFromJsonString<ShadowSocksServerObject>(x);
            *host = Server.address;
            *port = Server.port;
            return true;
        }
        else if (*protocol == "socks")
        {
            auto x = JsonToString(out["settings"].toObject()["servers"].toArray().first().toObject());
            auto Server = StructFromJsonString<SocksServerObject>(x);
            *host = Server.address;
            *port = Server.port;
            return true;
        }
        else
        {
            return false;
        }
    }

    const tuple<QString, QString, int> GetConnectionInfo(const ConnectionId &id, bool *status)
    {
        // TODO, what if is complex?
        if (status != nullptr)
            *status = false;
        auto root = ConnectionManager->GetConnectionRoot(id);
        return GetConnectionInfo(root, status);
    }

    const tuple<QString, QString, int> GetConnectionInfo(const CONFIGROOT &out, bool *status)
    {
        if (status != nullptr)
            *status = false;
        for (auto item : out["outbounds"].toArray())
        {
            OUTBOUND outBoundRoot = OUTBOUND(item.toObject());
            QString host;
            int port;
            QString outboundType = "";

            if (GetOutboundInfo(outBoundRoot, &host, &port, &outboundType))
            {
                if (status != nullptr)
                    *status = true;
                return { outboundType, host, port };
            }
            else
            {
                LOG(MODULE_CORE_HANDLER, "Unknown outbound type: " + outboundType + ", cannot deduce host and port.")
            }
        }
        return { QObject::tr("N/A"), QObject::tr("N/A"), 0 };
    }

    const tuple<quint64, quint64> GetConnectionUsageAmount(const ConnectionId &id)
    {
        auto connection = ConnectionManager->GetConnectionMetaObject(id);
        return { connection.upLinkData, connection.downLinkData };
    }

    uint64_t GetConnectionTotalData(const ConnectionId &id)
    {
        auto connection = ConnectionManager->GetConnectionMetaObject(id);
        return connection.upLinkData + connection.downLinkData;
    }

    int64_t GetConnectionLatency(const ConnectionId &id)
    {
        auto connection = ConnectionManager->GetConnectionMetaObject(id);
        return max(connection.latency, (int64_t) 0);
    }

    const QString GetConnectionProtocolString(const ConnectionId &id)
    {
        CONFIGROOT root = ConnectionManager->GetConnectionRoot(id);
        QString result;
        QStringList protocols;
        QStringList streamProtocols;
        auto outbound = root["outbounds"].toArray().first().toObject();
        result.append(outbound["protocol"].toString());

        if (outbound.contains("streamSettings"))
        {
            result.append(" / " + outbound["streamSettings"].toObject()["network"].toString());
            if (outbound["streamSettings"].toObject().contains("tls"))
            {
                result.append(outbound["streamSettings"].toObject()["tls"].toBool() ? " / tls" : "");
            }
        }

        return result;
    }
} // namespace Qv2ray::core
