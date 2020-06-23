#include "params/LruCache.hh"
#include "mem/port.hh"
#include "mem/mem_object.hh"

class LruCache: public MemObject
{
    class CPUSidePort: public SlavePort
    {
        private:
            LruCache *owner;

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
            void recvTimingReq(PacketPtr pkt) override;

            /**
             * @brief to receive a ask for another response
            */
            void recvRespRetry() override;

    }    

    class MemSidePort: public MasterPort
    {
        private:
            LruCache *owner;

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
    }

    public:

        /**
         * Construct
        */
        LruCache(LruCacheParams *p);

        MemSidePort memPort;

        CPUSidePort instrPort;
        CPUSidePort dataPort;

        /**
         * @param name the name of Port
         * @param idx
         * @return the master port to ask for
        */
        BaseMasterPort& getMasterPort(const std::string &name, PortID idx = InvalidPortID) override;

        /**
         * @param name the name of Port
         * @param idx
         * @return the slave port to ask for
        */
        BaseSlavePort& getSlavePort(constv std::string &name, PortID idx = InvalidPortID) override;

        /**
         * @param pkt the packet to handle when receive functional request from cpu
         */
        void handleFunctional(PacketPtr pkt);

        void handleRequest(PacketPtr pkt);

        /**
         * @brief tell cpu side the range of memory size
         */
        void sendRangeChange();

}
