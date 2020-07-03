#ifndef LRU_CACHE_HH
#define LRU_CACHE_HH

#include <vector>
#include <unordered_map>

#include "base/statistics.hh"
#include "mem/port.hh"
#include "params/LruCache.hh"
#include "sim/clocked_object.hh"

class LruCache: public ClockedObject
{
    private:
        class CPUSidePort: public SlavePort
        {
            private:
                // since in Lru cache, cpu ports make up vector so we need a specific id
                int id;

                // the cache belongs to
                LruCache *owner;

                // if we are waiting for the response of previous request, we need to retry the current request after receiving response
                bool needRetry;

                // if try to send a packet as response but blocked, just stored it here
                PacketPtr blockedPkt;

            public:
                /**
                 * constructor
                 */
                CPUSidePort(const std::string &name, int Id, LruCache *Owner):
                    SlavePort(name, Owner), \
                    id(Id), \
                    owner(Owner), \
                    needRetry(false), \
                    blockedPkt(nullptr)
                {
                    ;
                }

                /**
                 * @param pkt the packet to send
                 */
                void sendPacket(PacketPtr pkt);

                /**
                 * @brief get the range of the memory which is accessible
                 */
                AddrRangeList getAddrRanges() const override;

                /**
                 * @brief tell cpu it can give request again since all the previous procedure should be appropriately solved
                 */
                void trySendRetry();

            protected:
                /**
                 * @brief receive request from cpu in atomic mode
                 * @param pkt the pkt of request
                 */
                Tick recvAtomic(PacketPtr pkt) override
                {   panic("receive atomic unimpl");}

                /**
                 * @brief receive request from cpu in functional mode
                 * @param pkt the pkt of request
                 */
                void recvFunctional(PacketPtr pkt) override;
                
                /**
                 * @brief receive timing request
                 * @param pkt the pkt of request
                 * @return whether we can handle the request right now, if not, we may send retry if necessary
                 */
                bool recvTimingReq(PacketPtr pkt) override;

                /**
                 * @brief called by master port if response is not received appropriately and asked for response again
                 */
                void recvRespRetry() override;

        };

        class MemSidePort: public MasterPort
        {
            private:
                PacketPtr blockedPkt;

                LruCache *owner;

            public:
                /**
                 * constructor
                 */
                MemSidePort(const std::string &name, LruCache *Owner):
                    MasterPort(name, Owner), \
                    blockedPkt(nullptr), \
                    owner(Owner)
                {
                    ;
                }

                /**
                 * @param pkt the packet to send
                 */
                void sendPacket(PacketPtr pkt);

            protected:
                /**
                 * @param pkt the packet received
                 */
                bool recvTimingResp(PacketPtr pkt) override;

                /**
                 * @brief receive the retry of request
                 */
                void recvReqRetry() override;

                /**
                 * @brief receive the change of memory range, usually used when initialize
                 */
                void recvRangeChange() override;
        };

        /**
         * @param pkt the packet of request
         * @param port_id the port to deal with
         * @return true if we can handle it right now
         */
        bool handleRequest(PacketPtr pkt, int port_id);

        /**
         * @param pkt the packet as response
         * @return true if we can respond right now
         */
        bool handleResponse(PacketPtr pkt);

        /**
         * @brief put the request data from main memory to cache
         * @param pkt packet to insert into cache
         */
        void insert(PacketPtr pkt);

        /**
         * @brief send the response to cpu
         * @param pkt the packet to send to cpu
         */
        void sendResponse(PacketPtr pkt);

        /**
         * @brief deal with functional request
         * @param pkt the packet receive from the funtional request
         */
        void handleFunctional(PacketPtr pkt);
    
        AddrRangeList getAddrRanges() const;

        void sendRangeChange() const;

        /**
         * @param pkt the packet of request
         */
        void accessTiming(PacketPtr pkt);

        bool accessFunctional(PacketPtr pkt);

    public:
        LruCache(LruCacheParams *param);

        Port &getPort(const std::string &if_name, PortID idx = InvalidPortID) override;

        void regStats() override;

    private:
        // latency for both hit and miss
        const Cycles latency;

        // the size of block
        const unsigned blockSize;

        // the number of blocks in cache
        const unsigned capacity;

        // the ports of cpu side
        std::vector<CPUSidePort> cpuPorts;

        // the port of memory side
        MemSidePort memPort;

        // flag if cache is waiting for response
        bool blocked;

        // the packet is being handled currently
        PacketPtr originalPkt;

        // the id of the port to send the response when we receive
        int waitingPortId;

        // the cache map
        std::unordered_map<Addr, uint8_t*> cacheStore;

        // recorder for calculating miss latency
        Tick missTime;

        Stats::Scalar hits;
        Stats::Scalar misses;
        Stats::Histogram missLatency;
        Stats::Formula hitRatio;

};
#endif