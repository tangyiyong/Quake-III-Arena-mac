# 配置 App Store Connect API

这是开发者在本机创建的配置文件，不是苹果直接下载的 env 文件。

1. 登录 https://appstoreconnect.apple.com/ 。
2. 进入「用户和访问 → 集成 → App Store Connect API → 团队密钥」。
3. 尚未开通时由账号持有人申请 API 访问。账号持有人或管理员生成团队密钥，
   分配满足本次操作需要的角色权限。普通构建上传与初次注册标识/签名的权限需求可能不同。
4. 下载一次性的 `AuthKey_XXXXXXXXXX.p8`，记录 Key ID 和该页面的 Issuer ID。
5. 将私钥放到仓库外的 `~/.private_keys/`，在该目录创建 `appstoreconnect.env`：

```dotenv
ASC_KEY_ID=实际的Key_ID
ASC_ISSUER_ID=实际的Issuer_ID
ASC_KEY_PATH=/Users/你的用户名/.private_keys/AuthKey_实际Key_ID.p8
```

推荐权限：目录 `700`，env 和 p8 文件 `600`。不要提交 Git，不要粘贴私钥到聊天。
配置后只需告知 env 文件的本地路径；使用默认位置时告知“配置好了”即可。

Issuer ID 不是 Apple Developer Team ID。这个流程使用团队密钥，而非个人 API 密钥。

官方说明：https://developer.apple.com/help/app-store-connect/get-started/app-store-connect-api/
