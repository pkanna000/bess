#include "../module.h"

class MetadataTest : public Module {
 public:
  struct snobj *Init(struct snobj *arg);
  pb_error_t Init(const bess::MetadataTestArg &arg);

  void ProcessBatch(struct pkt_batch *batch);

  static const gate_idx_t kNumIGates = MAX_GATES;
  static const gate_idx_t kNumOGates = MAX_GATES;

  static const Commands<Module> cmds;

 private:
  struct snobj *AddAttributes(struct snobj *attrs, enum mt_access_mode mode);
  pb_error_t AddAttributes(
      const google::protobuf::Map<std::string, int64_t> &attrs,
      enum mt_access_mode mode);
};

const Commands<Module> MetadataTest::cmds = {};

pb_error_t MetadataTest::AddAttributes(
    const google::protobuf::Map<std::string, int64_t> &attrs,
    enum mt_access_mode mode) {
  for (const auto &kv : attrs) {
    int ret;

    const char *attr_name = kv.first.c_str();
    int attr_size = kv.second;

    ret = AddMetadataAttr(attr_name, attr_size, mode);
    if (ret < 0)
      return pb_error(-ret, "invalid metadata declaration");

    /* check /var/log/syslog for log messages */
    switch (mode) {
      case MT_READ:
        log_info("module %s: %s, %d bytes, read\n", name().c_str(), attr_name,
                 attr_size);
        break;
      case MT_WRITE:
        log_info("module %s: %s, %d bytes, write\n", name().c_str(), attr_name,
                 attr_size);
        break;
      case MT_UPDATE:
        log_info("module %s: %s, %d bytes, update\n", name().c_str(), attr_name,
                 attr_size);
        break;
    }
  }

  return pb_errno(0);
}

struct snobj *MetadataTest::AddAttributes(struct snobj *attrs,
                                          enum mt_access_mode mode) {
  if (snobj_type(attrs) != TYPE_MAP) {
    return snobj_err(EINVAL,
                     "argument must be a map of "
                     "{'attribute name': size, ...}");
  }

  /* a bit hacky, since there is no iterator for maps... */
  for (size_t i = 0; i < attrs->size; i++) {
    int ret;

    const char *attr_name = attrs->map.arr_k[i];
    int attr_size = snobj_int_get((attrs->map.arr_v[i]));

    ret = AddMetadataAttr(attr_name, attr_size, mode);
    if (ret < 0)
      return snobj_err(-ret, "invalid metadata declaration");

    /* check /var/log/syslog for log messages */
    switch (mode) {
      case MT_READ:
        log_info("module %s: %s, %d bytes, read\n", name().c_str(), attr_name,
                 attr_size);
        break;
      case MT_WRITE:
        log_info("module %s: %s, %d bytes, write\n", name().c_str(), attr_name,
                 attr_size);
        break;
      case MT_UPDATE:
        log_info("module %s: %s, %d bytes, update\n", name().c_str(), attr_name,
                 attr_size);
        break;
    }
  }

  return nullptr;
}

pb_error_t MetadataTest::Init(const bess::MetadataTestArg &arg) {
  pb_error_t err;

  err = AddAttributes(arg.read(), MT_READ);
  if (err.err() != 0) {
    return err;
  }

  err = AddAttributes(arg.write(), MT_WRITE);
  if (err.err() != 0) {
    return err;
  }

  err = AddAttributes(arg.update(), MT_UPDATE);
  if (err.err() != 0) {
    return err;
  }

  return pb_errno(0);
}

struct snobj *MetadataTest::Init(struct snobj *arg) {
  struct snobj *attrs;
  struct snobj *err;

  if ((attrs = snobj_eval(arg, "read"))) {
    err = AddAttributes(attrs, MT_READ);
    if (err) {
      return err;
    }
  }

  if ((attrs = snobj_eval(arg, "write"))) {
    err = AddAttributes(attrs, MT_WRITE);
    if (err) {
      return err;
    }
  }

  if ((attrs = snobj_eval(arg, "update"))) {
    err = AddAttributes(attrs, MT_UPDATE);
    if (err) {
      return err;
    }
  }

  return nullptr;
}

void MetadataTest::ProcessBatch(struct pkt_batch *batch) {
  /* This module simply passes packets from input gate X down
   * to output gate X (the same gate index) */
  RunChooseModule(get_igate(), batch);
}

ADD_MODULE(MetadataTest, "mt_test", "Dynamic metadata test module")
