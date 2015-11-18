#include <iostream>
#include "event.h"


using namespace std;


class cTimedEvent : public raven::event::cTimer
{
public:
    void handle_timer_event()
    {
        WaitThenPost( 300 );
        cout << "start " << id << "\n";
        std::this_thread::sleep_for (
            std::chrono::milliseconds( 1000 ));
        cout << "end   " << id << "\n\n";
    }
    int id;

};


int main()
{
    cTimedEvent te1;
    cTimedEvent te2;
    te1.id = 1;
    te2.id = 2;

    te1.WaitThenPost( 300 );
    te2.WaitThenPost( 300 );

    raven::event::cTimedStop stop( 5 );

    raven::event::theEventQueue.Run();

    return 0;
}
