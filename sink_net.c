// SPDX-License-Identifier: GPL-2.0-only
/* sink_net driver creates dummy network interface with sink0. All the packets
 * transmitted to this interface will be discarded and status count
 * will be updated.
 *
 * This driver also exposed PROCFS file "sink_net_status". Reading this
 * file gives the details of transmitted packets.
 *
 * $ cat /proc/sink_net_status
 * Total transmited packet count: 39 (4894 bytes)
 *
 * $ echo "0 0" > /proc/sink_net_status
 * # first number represents packet counts and second number represents bytes.
 */
#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>

#define SIOC_MODE_TX        (SIOCDEVPRIVATE+0)
#define SIOC_MODE_RX        (SIOCDEVPRIVATE+1)

/* Max chars in PROCFS tx status */
#define PROCFS_TX_STATUS_SIZE 256
/* Name of the PROCFS file entry */
#define PROCFS_TX_STATUS_ENTRY "sink_net_status"

static DEFINE_SPINLOCK(status_update_lock);
struct net_device *sink_ndev;
static uint32_t mode = SIOC_MODE_TX;

static int sink_net_open(struct net_device *dev)
{
	pr_info("%s called\n", __func__);
	/* Ready to accept transmit request */
	netif_start_queue(dev);
	return 0;
}

static int sink_net_release(struct net_device *dev)
{
	pr_info("%s called\n", __func__);
	netif_stop_queue(dev);
	return 0;
}

static int sink_net_init(struct net_device *dev)
{
	pr_info("SinkNet device initialized\n");
	return 0;
}

static int sink_net_xmit(struct sk_buff *skb, struct net_device *dev)
{
	unsigned long flags;

	pr_info("%s called, mode: %d\n", __func__, (mode % SIOCDEVPRIVATE));

	/* Take a update lock before updating values */
	spin_lock_irqsave(&status_update_lock, flags);
	if (mode == SIOC_MODE_TX) {
		/* Pass the packet to the hardware for transmission */
		dev->stats.tx_bytes += skb->len;
		dev->stats.tx_packets++;
	} else {
		/* Put the packet back into the network stack */
		//netif_rx(skb);
		dev->stats.rx_bytes += skb->len;
		dev->stats.rx_packets++;
	}

	spin_unlock_irqrestore(&status_update_lock, flags);

	skb_tx_timestamp(skb);
	dev_kfree_skb(skb);

	return NETDEV_TX_OK;
}

static int sink_net_ioctl(struct net_device *dev, struct ifreq *ifr, void __user *data, int cmd)
{
	pr_info("ioctl: command: %x\n", cmd);
	int ret = 0;

	switch (cmd) {
	case SIOC_MODE_TX:
		mode = SIOC_MODE_TX;
		pr_info("Mode updated to TX\n");
		break;
	case SIOC_MODE_RX:
		mode = SIOC_MODE_RX;
		pr_info("Mode updated to RX\n");
		break;
	default:
		pr_err("Invalid IOCTL cmd: %d", cmd);
		ret = -1;
	}
	return ret;
}

static ssize_t sink_net_proc_write(struct file *filp, const char __user *buffer,
					size_t length, loff_t *offset)
{
	int ret = 0;
	char msg[PROCFS_TX_STATUS_SIZE];
	ssize_t packets = 0;
	ssize_t bytes = 0;
	ssize_t msg_len = 0;
	unsigned long flags;

	ret = copy_from_user(msg, buffer, length);
	if (ret) {
		pr_err("Failed to copy buffer from user to kernel space. Total copied bytes: %ld\n", length - ret);
		return -EFAULT;
	}

	ret = sscanf(msg, "%ld %ld", &packets, &bytes);
	if (ret != 2) {
		pr_err("Failed to extract packet count and bytes from buffer.\n");
		return -EFAULT;
	}

	pr_info("Updating tx packet status. packets: %ld, bytes: %ld\n", packets, bytes);

	/* Take a update lock before updating values */
	spin_lock_irqsave(&status_update_lock, flags);

	if (mode == SIOC_MODE_TX) {
		/* TX mode */
		sink_ndev->stats.tx_packets = packets;
		sink_ndev->stats.tx_bytes = bytes;
	} else if (mode == SIOC_MODE_RX) {
		/* RX mode */
		sink_ndev->stats.rx_packets = packets;
		sink_ndev->stats.rx_bytes = bytes;
	}

	spin_unlock_irqrestore(&status_update_lock, flags);

	msg_len = strlen(msg);
	*offset = msg_len;
	return msg_len;
}

static ssize_t sink_net_proc_read(struct file *filp, char __user *buffer,
					size_t length, loff_t *offset)
{
	int ret = 0;
	int len = 0;
	char tx_status_msg[PROCFS_TX_STATUS_SIZE];

	if (*offset > 0)
		return 0;

	if (mode == SIOC_MODE_TX)
		len += sprintf(tx_status_msg + len, "Currently Running in mode: TX\n");
	else if (mode == SIOC_MODE_RX)
		len += sprintf(tx_status_msg + len, "Currently Running in mode: RX\n");

	len += sprintf(tx_status_msg + len, "Total transmited packet count: %ld (%ld bytes)\n",
		sink_ndev->stats.tx_packets, sink_ndev->stats.tx_bytes);
	len += sprintf(tx_status_msg + len, "Total received packet count: %ld (%ld bytes)",
		sink_ndev->stats.rx_packets, sink_ndev->stats.rx_bytes);

	tx_status_msg[len++] = '\0';

	ret = copy_to_user(buffer, tx_status_msg, len);
	if (ret) {
		pr_err("Failed to copy buffer from kernel to user space. Total copied bytes: %d\n", len - ret);
		return 0;
	}
	*offset = len;

	return len;
}

static loff_t sink_net_llseek(struct file *filp, loff_t offset, int whence)
{

	switch (whence) {
	case SEEK_SET: case SEEK_CUR: case SEEK_END:
		return generic_file_llseek(filp, offset, whence);
	default:
		return -EINVAL;
	}
}

static const struct proc_ops proc_fops = {
	.proc_read   = sink_net_proc_read,
	.proc_write  = sink_net_proc_write,
	.proc_lseek  = sink_net_llseek,
};

const struct net_device_ops sink_netdev_ops = {
	.ndo_init = sink_net_init,
	.ndo_open = sink_net_open,
	.ndo_stop = sink_net_release,
	.ndo_start_xmit = sink_net_xmit,
	//.ndo_do_ioctl = sink_net_ioctl,
	.ndo_siocdevprivate = sink_net_ioctl,
};

static void sink_setup(struct net_device *dev)
{
	ether_setup(dev);
	dev->netdev_ops = &sink_netdev_ops;
}

static int __init sink_net_mod_init(void)
{
	int ret = 0;
	struct proc_dir_entry *ent;

	sink_ndev = alloc_netdev(0, "sink%d", NET_NAME_ENUM, sink_setup);
	if (!sink_ndev)
		return -ENOMEM;

	ret = register_netdevice(sink_ndev);
	if (ret < 0) {
		pr_err("Failed to register sink net device. return code: %d\n", ret);
		goto err;
	}

	/* create procfs entry */
	ent = proc_create(PROCFS_TX_STATUS_ENTRY, 0666, NULL, &proc_fops);
	if (ent == NULL) {
		pr_err("Failed to create sink_net_status procfs entry.\n");
		ret = -ENOENT;
		goto err1;
	}
	return 0;

err1:
	unregister_netdev(sink_ndev);
err:
	free_netdev(sink_ndev);
	return ret;
}

static void __exit sink_net_mod_cleanup(void)
{
	pr_info("Cleaning up sink net module\n");
	remove_proc_entry("sink_net_status", NULL);
	unregister_netdev(sink_ndev);
}

module_init(sink_net_mod_init);
module_exit(sink_net_mod_cleanup);
MODULE_DESCRIPTION("Dummy netdevice driver which accepts all skb packets and discard them.");
MODULE_AUTHOR("Sanket Parmar <sanketsparmar@gmail.com>");
MODULE_LICENSE("GPL v2");
