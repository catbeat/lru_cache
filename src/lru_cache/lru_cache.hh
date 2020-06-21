#include "params/LruCache.hh"
#include "mem/port.hh"
#include "mem/mem_object.hh"

class LruCache: public MemObject
{
    private:
        class CPUSidePort: public SlavePort
        {
            private:
                LruCache *owner;

                /* needRetry flag when a request received when a previous request is still outstanding
                * once the previous request is over, we should retry for the request
                */ 
                bool needRetry;

                /**
                 * to store blocked Packet when memport failed to send Timing request so that we can send again when receiving request retry
                 */
                PacketPtr blockedPacket;

            public:
                /**
                 * Construct
                */
                CPUSidePort(const std::string &name, LruCache* Owner)
                  :SlavePort(name, Owner), owner(Owner)
                {
                  ;
                }

                /**
                 * @brief get the range of accessed address
                */
                AddrRangeList getAddrRanges() const override;

            protected:

                /**
                 * @param pkt the packet sent from atomic cpu mode
                */
                Tick recvAtomic(PacketPtr pkt) override
                {
                  ;
                }

                /**
                 * @param pkt the packet sent from functional cpu mode
                */
                void recvFunctional(PacketPtr pkt) override;

                /**
                 * @param pkt the packet sent from timing cpu mode
                */
                bool recvTimingReq(PacketPtr pkt) override;

                /**
                 * @brief to receive a ask for another response
                */
                void recvRespRetry() override;

                /**
                 * @brief send the packet as response to CPU
                 * @param pkt the packet ptr to send
                 */
                void sendPacket(PacketPtr pkt);

                /**
                 * @brief reask for request when previously we blocked request
                 */ 
                void trySendRetry();

        };    

        class MemSidePort: public MasterPort
        {
            private:
                LruCache *owner;

                /**
                 * to store blocked Packet when memport failed to send Timing request so that we can send again when receiving request retry
                 */
                PacketPtr blockedPacket;

            public:
                
                /**
                 * Construct
                */
                MemSidePort(const std::string &name, LruCache *Owner)
                  :MasterPort(name, Owner), owner(Owner)
                {
                  ;
                }

                /**
                 * @param pkt packet to send
                */
                void sendPacket(PacketPtr pkt);

            protected:
            
                /**
                 * @param pkt packet received from Timing CPU
                */
                bool recvTimingResp(PacketPtr pkt) override;

                /**
                 * @brief receive the signal from memory to send request again
                */
                void recvReqRetry() override;

                /**
                 * @brief receive the access range to change, always called for first init
                */
                void recvRangeChange() override;
        };

        MemSidePort memPort;

        CPUSidePort instrPort;
        CPUSidePort dataPort;

        /**
         * blocked flag when memPort is sending packet and waiting for response, meanwhile we can't handle other request
         */
        bool blocked;

        /**
         * @param pkt the packet to handle when receive functional request from cpu
         */
        void handleFunctional(PacketPtr pkt);

        /**
         * @param pkt the packet of current request which we need to handle
         * @return true if we can handle it now otherwise we have to wait for the response of a previous one
         */
        bool handleRequest(PacketPtr pkt);

        /**
         * @param 
         */
        bool handleResponse(PacketPtr pkt);

        /**
         * @brief tell cpu side the range of memory size
         */
        void sendRangeChange();

    public:

        /**
         * Construct
        */
        LruCache(LruCacheParams *p);

        /**
         * @param name the name of Port
         * @param idx
         * @return the master port to ask for
        */
        MasterPort& getMasterPort(const std::string &name, PortID idx = InvalidPortID) override;

        /**
         * @param name the name of Port
         * @param idx
         * @return the slave port to ask for
        */
        SlavePort& getSlavePort(const std::string &name, PortID idx = InvalidPortID) override;

};
