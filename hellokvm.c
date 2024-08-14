#include <linux/kvm.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include "code.h"

#define RAM_SIZE 128*1024*1024

int main(int argc, char **argv)
{
	struct kvm_userspace_memory_region mem;
	struct kvm_run *vcpu;
	int kvmfd, vmfd, vcpufd;
	int vcpu_size;
	void *vmram;

	kvmfd = open("/dev/kvm", O_RDWR);
	printf("kvmfd: %d\n", kvmfd);
	vmfd = ioctl(kvmfd, KVM_CREATE_VM, 0);
	printf("vmfd: %d\n", vmfd);
	vmram = mmap(NULL, RAM_SIZE, PROT_READ | PROT_WRITE,
			MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	printf("vmram: 0x%lx\n", (long)vmram);
	memcpy(vmram, guest_code, sizeof(guest_code));

	mem.slot = 0;
	mem.flags = 0;
	mem.guest_phys_addr = 0;
	mem.memory_size = RAM_SIZE;
	mem.userspace_addr = (__u64) vmram;

	ioctl(vmfd, KVM_SET_USER_MEMORY_REGION, &mem);
	vcpufd = ioctl(vmfd, KVM_CREATE_VCPU, 0);
	printf("vcpufd: %d\n", vcpufd);
	vcpu_size = ioctl(kvmfd, KVM_GET_VCPU_MMAP_SIZE, 0);
	printf("vcpu_size: %d\n", vcpu_size);
	vcpu = mmap(NULL, vcpu_size, PROT_READ | PROT_WRITE,
			MAP_SHARED, vcpufd, 0);
	printf("vcpu: 0x%lx\n", (long)vcpu);

	while (1) {
		ioctl(vcpufd, KVM_RUN, 0);

		switch (vcpu->exit_reason) {
		case KVM_EXIT_INTERNAL_ERROR:
			printf("internal_error\n");
			break;
		case KVM_EXIT_IO:
			printf("data: %c\n", *(int *)((char *)vcpu + vcpu->io.data_offset));
			break;
		case KVM_EXIT_HLT:
			printf("halt\n");
			return 0;
		default:
			printf("exit_reason: 0x%x\n", vcpu->exit_reason);
		}
	}
}
