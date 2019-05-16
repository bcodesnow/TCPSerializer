#include <QCoreApplication>

class TCPSerializer
{
private:
    int                 sock;
    pthread_t           thread_c;
    bool*               start_flag;

    typedef struct job_type {
    char*            data_ptr;
    uint32_t*           data_length_ptr;
    pthread_mutex_t*    mutex_ptr;
    uint8_t*             buff_state_ptr;
    }   job_type;

    typedef void * (*THREADFUNCPTR)(void *);

    vector<job_type> job_queue;

    void* streamClient(void* arg)
    {
        UNUSED(arg);
        struct sockaddr_in serv_addr;
        int fd;
        if ( ( fd = socket(AF_INET, SOCK_STREAM, 0 ) ) < 0 )
            errno_abort("socket");
        /* make this thread cancellable using pthread_cancel() */
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
        pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

        bzero(&servaddr, sizeof(servaddr));

        // assign IP, PORT
        servaddr.sin_family = AF_INET;
        servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
        servaddr.sin_port = htons(PORT);

        // Binding newly created socket to given IP and verification
        if ((bind(sockfd, (SA*)&servaddr, sizeof(servaddr))) != 0)
        {
            printf("socket bind failed...\n");
            exit(0);
        }
        else
        printf("Socket successfully bound..\n");

        // Now server is ready to listen and verification
        if ( ( listen( sockfd, 5 ) ) != 0 )
        {
            printf("Listen failed...\n");
            exit(0);
        }
        else
        {
            printf("Server listening..\n");
        }
        len = sizeof(cli);

        // Accept the data packet from client and verification
        connfd = accept(sockfd, (SA*)&cli, &len);
        if (connfd < 0) {
            printf("server accept failed...\n");
            exit(0);
        }
        else
            printf("server accept the client...\n");




        int counter = 0;
        struct timespec nsleep;
        //nsleep.tv_nsec = 500000; //0,5ms
        nsleep.tv_nsec = 750000; //1,25ms

        *start_flag = TRUE;
        while(1)
        {
            /* send the frame, thread safe */
            if (job_queue.size())
            {
                static job_type t_job = job_queue.back();
                job_queue.pop_back();
                pthread_mutex_lock(t_job.mutex_ptr);

                int pkg_cnt = *t_job.data_length_ptr / PKG_SIZE;
                int last_pkg_size = *t_job.data_length_ptr % PKG_SIZE;
                int it =0;
                for (; it < pkg_cnt; it++)
                {
                    if (sendto(fd, &t_job.data_ptr[it*PKG_SIZE], PKG_SIZE, 0, (struct sockaddr*) &send_addr, sizeof send_addr) < 0)
                    {
                        quit("\n--> send() failed", 1);
                        errno_abort("send");
                    }
                    else {

                    }
                }
                if (last_pkg_size > 0)
                {
                    if (sendto(fd, &t_job.data_ptr[it*PKG_SIZE], last_pkg_size, 0, (struct sockaddr*) &send_addr, sizeof send_addr) < 0)
                    {
                        quit("\n--> send() failed", 1);
                        errno_abort("send");
                    }
                }
                *t_job.buff_state_ptr = BUF_STATE_READY_FOR_REUSE;
                pthread_mutex_unlock(t_job.mutex_ptr);
//                if ( counter++ % 150 == 0)
//                    std::cout<<counter<<" Frames Sent."<<std::endl;
            }

            /* no, take a rest for a while */
            else
            {
            nanosleep(&nsleep, NULL);   //
            }
            /* have we terminated yet? */
            pthread_testcancel();
        }
    }

public:
    // this is crazy shit that is possible to call thread create in a constructor on a non static member function!
    UDPStreamer(bool* t_start_flag)
    {
        start_flag = t_start_flag;
        if (pthread_create(&this->thread_c, NULL, (THREADFUNCPTR) &UDPStreamer::streamClient, this))
            quit("\n--> pthread_create failed.", 1);
    }
    // use add_job to hand over send requests, non blocking. The job is done, when the t_buff_state_ptr becomes READY_FOR_REUSE peek on it by trying to unlock the mutex.
    void add_job (pthread_mutex_t* t_mutex_ptr, char* t_data_ptr, uint32_t* t_data_length_ptr, uint8_t* t_buff_state_ptr)
    {
        /*
        typedef struct job_type {
        uint8_t*            data_ptr;
        uint32_t*           data_length_ptr;
        pthread_mutex_t*    mutex_ptr;
        uint8_t*             buff_state_ptr;
        }   job_type;
        */
        this->job_queue.insert( job_queue.begin(),  { t_data_ptr, t_data_length_ptr, t_mutex_ptr, t_buff_state_ptr });
    }

    void quit(string msg, int retval)
    {
        if (retval == 0) {
            cout << (msg == "NULL" ? "" : msg) << "\n" << endl;
        } else {
            cerr << (msg == "NULL" ? "" : msg) << "\n" << endl;
        }
        if (clientSock){
            close(clientSock);
        }
        // todo: add destroy buffers and self
        exit(retval);
    }

    void errno_abort(const char* header)
    {
        perror(header);
        exit(EXIT_FAILURE);
    }
};




int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    return a.exec();
}
