/*
MIT License

Copyright(c) 2020 Futurewei Cloud

    Permission is hereby granted,
    free of charge, to any person obtaining a copy of this software and associated documentation files(the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and / or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions :

    The above copyright notice and this permission notice shall be included in all copies
    or
    substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS",
    WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
    DAMAGES OR OTHER
    LIABILITY,
    WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
*/

#pragma once

#include <k2/transport/PayloadSerialization.h>
#include <seastar/core/sharded.hh>
#include <k2/transport/Payload.h>
#include <k2/transport/Status.h>
#include <k2/common/Common.h>
#include <k2/config/Config.h>
#include <k2/cpo/client/CPOClient.h>
#include <k2/transport/BaseTypes.h>
#include <k2/transport/TXEndpoint.h>
#include <k2/persistence/plog_client/PlogClient.h>


namespace k2 {

class LogStreamMgr{
public:
    LogStreamMgr();
    ~LogStreamMgr();

    // initializate the plog client it holds
    seastar::future<> init(String cpo_url, String persistenceClusrerName);

    // create a log stream with a given name
    seastar::future<> create(String logStreamName);

    // write data to the log stream
    seastar::future<> write(Payload payload);

    // TODO: read the data from the log stream with given offset and size
    seastar::future<std::vector<Payload> > read_WAL(String logStreamName, uint32_t offset, uint32_t size);

    // read all the data from the log stream, this is only for test propose
    seastar::future<std::vector<Payload> > read_all_WAL(String logStreamName);

private:
    // the maximum size of each plog
    constexpr static uint32_t PLOG_MAX_SIZE = 16 * 1024 * 1024;
    // How many WAL plogs it will create in advance 
    constexpr static uint32_t WAL_PLOG_POOL_SIZE = 1;
    // How many metadata plogs it will create in advance
    constexpr static uint32_t METADATA_PLOG_POOL_SIZE = 1;

    CPOClient _cpo;
    PlogClient _client;
    String _logStreamName;

    // TODO: decouple the WALLogstreamMgr and the MetadataLogStreamMgr
    std::vector<String> _walPlogPool; // The vector to store the created WAL plog Id
    std::vector<String> _metadataPlogPool; // the vector to store the created Metadata plog Id
    std::pair<String, uint32_t> _walInfo;
    std::pair<String, uint32_t> _metaInfo;
    
    bool _switched;
    std::vector<seastar::promise<>> _requestWaiters;

    String _ongoindWALSealedPlog;
    String _ongoindMetadataSealedPlog;
    bool _logStreamCreate = false; // Whether this logstream has been created 

    // write data to the log stream. writeToWAL == false means write to the metadata log stream, while writeToWAL == true means write to the WAL
    seastar::future<> _write(Payload payload, bool writeToWAL);
    // when exceed the size limit of current plog, we need to seal the current plog, write the sealed offset to metadata, and write the contents to the new plog
    seastar::future<> _switchPlogAndWrite(Payload payload, bool writeToWAL);
    // read the contents from WAL plogs
    seastar::future<std::vector<Payload> > _readContent(std::vector<Payload> payloads);
    ConfigDuration _cpo_timeout {"cpo_timeout", 1s};
};

} // k2