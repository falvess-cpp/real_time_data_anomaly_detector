FROM redis:7-alpine

# Copy the specialized local network and memory optimization profile configuration
COPY redis.conf /usr/local/etc/redis/redis.conf

# Expose port 6380 matching the target ports defined inside the engine source code
EXPOSE 6380

# Run Redis server using the custom embedded configuration profile
CMD [ "redis-server", "/usr/local/etc/redis/redis.conf" ]