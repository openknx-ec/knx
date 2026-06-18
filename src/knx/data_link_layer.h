#pragma once

#include "config.h"

#include <stdint.h>
#include "device_object.h"
#include "knx_types.h"
#include "network_layer_entity.h"
#include "cemi_server.h"
#include "bau.h"
#include "ip_tunnel_server.h"

class Platform;

typedef void (*ActivityCallback)(uint8_t info);

/**
 * Passive bus-monitor observer.
 *
 * Invoked for every frame the data link layer receives, before the network/
 * application layer applies any address filter. `frame` is only valid for the
 * duration of the call (it references stack-internal buffers) - copy out what
 * you need. `monitorMode` is true when the frame was delivered by the true
 * bus-monitor mode (passive, device off the bus) and false when it is a passive
 * observation during normal operation. The callback must be short and must not
 * block; it runs in the stack loop context.
 */
typedef void (*BusmonitorCallback)(CemiFrame& frame, bool monitorMode);

class DataLinkLayerCallbacks
{
protected:
    ActivityCallback _activityCallback = nullptr;
public:
    virtual ~DataLinkLayerCallbacks() = default;
    virtual void activity(uint8_t info);
    virtual void setActivityCallback(ActivityCallback activityCallback);
};

class DataLinkLayer
{
  public:
    DataLinkLayer(DeviceObject& devObj, NetworkLayerEntity& netLayerEntity,
                  Platform& platform, BusAccessUnit& busAccessUnit
#ifdef KNX_TUNNELING
                  ,IpTunnelServer& ipTunnelServer
#endif
                );

#ifdef USE_CEMI_SERVER
    // from tunnel
    void cemiServer(CemiServer& cemiServer);
    void dataRequestFromTunnel(CemiFrame& frame);
#ifdef KNX_TUNNELING_
    virtual void dataRequestToTunnel(CemiFrame& frame);
    virtual void dataConfirmationToTunnel(CemiFrame& frame);
    virtual void dataIndicationToTunnel(CemiFrame& frame);
    virtual bool isTunnelAddress(uint16_t addr);
#endif
#endif

    // from network layer
    void dataRequest(AckType ack, AddressType addrType, uint16_t destinationAddr, uint16_t sourceAddr, FrameFormat format,
                     Priority priority, NPDU& npdu, bool doNotRepeat);
    void systemBroadcastRequest(AckType ack, FrameFormat format, Priority priority, NPDU& npdu, uint16_t sourceAddr,  bool doNotRepeat);
    virtual void loop() = 0;
    virtual void enabled(bool value) = 0;
    virtual bool enabled() const = 0;
    virtual DptMedium mediumType() const = 0;

    // Passive bus-monitor hook (use case b): register (or clear with nullptr)
    // an observer that is invoked for every received frame, before address
    // filtering. Registering a callback does NOT change ACK/routing behaviour
    // and is safe to call at runtime.
    void setBusmonitorCallback(BusmonitorCallback callback);
    BusmonitorCallback busmonitorCallback() const;

  protected:
    void frameReceived(CemiFrame& frame);
    // Notify the registered busmonitor observer, if any. Returns true when a
    // callback was invoked. `monitorMode` distinguishes true monitor mode (a)
    // from passive sniffing in normal operation (b).
    bool notifyBusmonitor(CemiFrame& frame, bool monitorMode);
    void dataConReceived(CemiFrame& frame, bool success);
    bool sendTelegram(NPDU& npdu, AckType ack, uint16_t destinationAddr, AddressType addrType, uint16_t sourceAddr, FrameFormat format, Priority priority, SystemBroadcast systemBroadcast, bool doNotRepeat = false);
    virtual bool sendFrame(CemiFrame& frame) = 0;
    uint8_t* frameData(CemiFrame& frame);
    DeviceObject& _deviceObject;
    NetworkLayerEntity& _networkLayerEntity;
    Platform& _platform;
    BusAccessUnit& _bau;
    BusmonitorCallback _busmonitorCallback = nullptr;
#ifdef USE_CEMI_SERVER
    CemiServer* _cemiServer;
#endif
#ifdef KNX_ACTIVITYCALLBACK
    uint8_t _netIndex = 0;
#endif
#ifdef KNX_TUNNELING
    IpTunnelServer& _ipTunnelServer;
  public:
    bool isTunnelingPA(uint16_t pa);
    bool isRoutedPA(uint16_t pa);
#endif
};
