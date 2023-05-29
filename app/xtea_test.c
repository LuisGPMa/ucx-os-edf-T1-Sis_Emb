#include <ucx.h>

struct pipe_s *encrypt_pipe, *decrypt_pipe;

void encrypt()
{
	char message[128];

    for (;;) {
        memset(message, 0, sizeof(message));
        printf("Waiting message from task0... ");
        while (ucx_pipe_size(encrypt_pipe) < 1);
	    ucx_pipe_read(encrypt_pipe, message, ucx_pipe_size(encrypt_pipe));
	    printf("message: %s\n", message);
    }

}

void string2hexString(char* input, char* output)
{
    int loop;
    int i;

    i = 0;
    loop = 0;

    while (input[loop] != '\0') {
        sprintf((char*)(output + i), "%02X", input[loop]);
        loop += 1;
        i += 2;
    }
    //insert NULL at the end of the output string
    output[i++] = '\0';
}

void task0(void)
{
    char* encrypt_this = "Mensagem secreta";
	while (1) {
		/* write pipe - write size must be less than buffer size */
		ucx_pipe_write(encrypt_pipe, encrypt_this, strlen(encrypt_this));
	}
}

int32_t app_main(void)
{
	ucx_task_add(encrypt, DEFAULT_STACK_SIZE);
	ucx_task_add(task0, DEFAULT_STACK_SIZE);

	encrypt_pipe = ucx_pipe_create(128);		/* pipe buffer, 128 bytes (allocated on the heap) */
	decrypt_pipe = ucx_pipe_create(128);		/* pipe buffer, 64 bytes */

	// start UCX/OS, preemptive mode
	return 1;
}