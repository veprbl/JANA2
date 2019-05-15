#ifndef JTestEventProcessor_h
#define JTestEventProcessor_h

#include <JANA/JApplication.h>
#include <JANA/JEventProcessor.h>
#include <JANA/JEvent.h>

class JTestEventProcessor : public JEventProcessor {

private:
    size_t m_cputime_ms = 10;
    size_t m_write_bytes = 2000000;
    double m_cputime_spread = 0.25;
    double m_write_spread = 0.25;

public:

    JTestEventProcessor(JApplication* app) : JEventProcessor(app) {

        auto params = japp->GetJParameterManager();

        params->SetDefaultParameter("jtest:plotter_bytes", m_write_bytes, "");
        params->SetDefaultParameter("jtest:plotter_ms", m_cputime_ms, "");
        params->SetDefaultParameter("jtest:plotter_bytes_spread", m_write_spread, "");
        params->SetDefaultParameter("jtest:plotter_spread", m_cputime_spread, "");
    }

    ~JTestEventProcessor() {}

    const char* className() {
        return "JTestEventProcessor";
    }

    void Init() {
    }

    void Process(const std::shared_ptr<const JEvent>& aEvent) {

        // Read the track data
        auto td = aEvent->GetSingle<JTestTrackData>();
        auto sum = read_memory(td->buffer);

        // Consume CPU
        consume_cpu_ms(m_cputime_ms, m_cputime_spread);

        // Write the histogram data
        auto hd = new JTestHistogramData;
        write_memory(hd->buffer, m_write_bytes, m_write_spread);
        aEvent->Insert(hd);
    }

    void Finish() {}

};

#endif // JTestEventProcessor

