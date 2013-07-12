#include "EventSourceStreamReader.h"

namespace MicrosoftAspNetSignalRClientCpp
{

EventSourceStreamReader::EventSourceStreamReader(shared_ptr<Connection> connection, Concurrency::streams::basic_istream<uint8_t> stream)
    : AsyncStreamReader(stream)
{
    pBuffer = unique_ptr<ChunkBuffer>(new ChunkBuffer());

    SetDataCallback([this](shared_ptr<char> readBuffer){
        ProcessBuffer(readBuffer);
    });
}


EventSourceStreamReader::~EventSourceStreamReader()
{
}

void EventSourceStreamReader::ProcessBuffer(shared_ptr<char> readBuffer)
{
    {
        lock_guard<mutex> lock(mBufferLock);

        pBuffer->Add(readBuffer);

        while(pBuffer->HasChuncks())
        {
            string_t line = pBuffer->ReadLine();

            if (line.empty())
            {
                continue;
            }

            shared_ptr<SseEvent> sseEvent;
            if (!SseEvent::TryParse(line, &sseEvent))
            {
                continue;
            }

            if (auto pConnection = wpConnection.lock())
            {
                wstringstream ss;
                ss << "SSE: OnMessage(" << sseEvent->ToString() << ")";
                pConnection->Trace(TraceLevel::Messages, ss.str());
            }

            OnMessage(sseEvent);
        }
    }
}

void EventSourceStreamReader::OnMessage(shared_ptr<SseEvent> sseEvent)
{
    if (Message != nullptr)
    {
        lock_guard<mutex> lock(mMessageLock);
        Message(sseEvent);
    }
}


void EventSourceStreamReader::SetMessageCallback(function<void(shared_ptr<SseEvent> sseEvent)> message)
{
    lock_guard<mutex> lock(mMessageLock);
    Message = message;
}

} // namespace MicrosoftAspNetSignalRClientCpp