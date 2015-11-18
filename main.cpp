#include <iostream>
#include "event.h"


using namespace std;

/**  Demonstration class */

class cTimedEvent : public raven::event::cTimer
{
public:

    /** Function to post when the timer fires

    This demonstrates that the timers run asynchronously
    ( in parrallel and without interfering with the executing application code )
    but the event handlers run synchronously.

    The output should show that each event handler runs to completion
    once started before the next event handler starts,
    even though the event handler reuire 1 second to complete, but are schedule to run ever 300 msecs.

    The advantage of this is that the application code need not worry about contention between the
    different handlers, no matter when thet are posted to run.

    */
    void handle_timer_event()
    {
        // First, schedule posting another copy of this function in 300 ms
        WaitThenPost( 300 );

        // Let user know we started
        cout << "start " << id << "\n";

        // simulate doing some work for an etire second
        std::this_thread::sleep_for (
            std::chrono::milliseconds( 1000 ));

        // tell user that we are done
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
